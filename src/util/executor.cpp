/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <arrow/array.h>
#include <arrow/record_batch.h>
#include <arrow/result.h>
#include <memory>
#include <parquet/arrow/reader.h>
#include <ranges>
#include <string_view>

#include "mzpeak/schema/group.h"
#include "mzpeak/util/algorithm.h"
#include "mzpeak/util/batch.h"
#include "mzpeak/util/decoders.h"
#include "mzpeak/util/executor.h"
#include "mzpeak/util/projection.h"
#include "mzpeak/util/query.h"
#include "mzpeak/util/slice.h"
#include "mzpeak/util/types.h"

namespace MzPeak::Util {

/******************************************************************************/
struct Executor::Impl {
  Impl(std::shared_ptr<parquet::arrow::FileReader> reader, const Projection& fields)
      : reader_(reader)
      , projection_(fields)
  {
  }

  /// Helper to check a result and throw an error if necessary.
  template <typename T> T check(std::string_view msg, arrow::Result<T> r)
  {
    if (!r.ok()) {
      std::string error("while executing a query: " + std::string(msg) + ": ");
      throw ParquetError(error + r.status().ToString());
    }

    return std::move(r.ValueOrDie());
  }

  /// Run a query on the given record batch and return a vector
  /// indicating which rows should be kept.
  std::vector<bool> filter(const Query&, std::shared_ptr<arrow::RecordBatch>&);

  /// Helper to extract a single value from an array.
  struct ArrayValueHelper {
    template <Type T> Query::Result<Query::value_t> operator()();
    Executor::Impl& impl_;
    const Schema::Column& field_;
    std::shared_ptr<arrow::RecordBatch>& batch_;
    int64_t row_index_;
  };

  std::shared_ptr<parquet::arrow::FileReader> reader_;
  Projection projection_;
};

/******************************************************************************/
template <Type T>
Query::Result<Query::value_t> Executor::Impl::ArrayValueHelper::operator()()
{
  Batch helper(batch_);
  std::shared_ptr<arrow::Array> a(helper.raw(field_));

  if (row_index_ < 0 || row_index_ >= a->length() || a->IsNull(row_index_)) {
    return Query::Result<Query::value_t>::skip();
  }

  auto casted = std::static_pointer_cast<typename type_traits<T>::array_type>(a);
  auto value = Decoders::unsafe_array_value<T>(casted, row_index_);
  return Query::Result<Query::value_t>(value);
}

/******************************************************************************/
std::vector<bool> Executor::Impl::filter(const Query& q,
                                         std::shared_ptr<arrow::RecordBatch>& batch)
{
  std::vector<bool> want_rows(batch->num_rows(), true);

  auto get_value =
      [&](int64_t row_index,
          const Schema::Column& field) -> Query::Result<Query::value_t> {
    if (field.second->type().has_value()) {
      return lift_type(field.second->type().value(),
                       ArrayValueHelper{*this, field, batch, row_index});
    } else {
      return Query::Result<Query::value_t>::fail();
    }
  };

  for (int64_t i : std::views::iota(0, batch->num_rows())) {
    Query::Result<bool> res = q.eval(std::bind(get_value, i, std::placeholders::_1));
    want_rows[i] = !res.is(false);
  }

  return want_rows;
}

/******************************************************************************/
Executor::Executor(std::shared_ptr<parquet::arrow::FileReader> reader,
                   const Projection& fields)
    : impl_(std::make_unique<Impl>(std::move(reader), fields))
{
}

/******************************************************************************/
Executor::~Executor() = default;

/******************************************************************************/
std::generator<const Batch&> Executor::read(const Query& q, const Plan& plan) const
{
  // Reduce a batch to just those rows that were selected by the query.
  auto narrow = [&](std::size_t i, std::size_t j,
                    std::shared_ptr<arrow::RecordBatch>& rb)
      -> std::shared_ptr<arrow::RecordBatch> {
    int64_t offset = i;
    int64_t length = j - i + 1;
    if (length <= 0) return rb;
    return rb->Slice(offset, length);
  };

  std::unique_ptr<arrow::RecordBatchReader> batch_reader = nullptr;

  if (plan.is_full_scan()) {
    // FIXME: We should emit a warning here.
    batch_reader =
        impl_->check("invalid batch reader", impl_->reader_->GetRecordBatchReader());
  } else {
    batch_reader = impl_->check(
        "invalid batch reader",
        impl_->reader_->GetRecordBatchReader(plan.materalize_as_vector()));
  }

  for (auto batch_r : *batch_reader) {
    auto batch = impl_->check("invalid record batch", batch_r);
    auto want_rows = impl_->filter(q, batch);

    for (const auto& span : Algorithm::spans(want_rows)) {
      auto nb = narrow(span.first, span.second, batch);
      Batch val(nb);
      co_yield val;
    }
  }
}

/******************************************************************************/
std::unique_ptr<Slice> Executor::execute(const Query& q, const Plan& plan)
{
  std::unique_ptr<Slice> slice(new Slice(impl_->projection_.get()));

  for (const auto& batch : read(q, plan)) {
    for (const auto& field : slice->fields()) {
      slice->append(field, batch.raw(field));
    }
  }

  return slice;
}

} // namespace MzPeak::Util
