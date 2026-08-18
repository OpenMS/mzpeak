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

#include "mzpeak/util/algorithm.h"
#include "mzpeak/util/executor.h"
#include "mzpeak/util/types.h"

namespace MzPeak::Util {

/******************************************************************************/
struct Executor::Impl {
  Impl(std::shared_ptr<parquet::arrow::FileReader> reader, const Projection& fields)
      : reader_(reader)
      , projection_(fields)
      , slice_(nullptr)
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
  std::vector<bool> filter(const Planner::Plan&,
                           std::shared_ptr<arrow::RecordBatch>&);

  /// Capture the requested columns.
  void project(std::shared_ptr<arrow::RecordBatch>&);

  /// Return the array associated with the given field.
  std::shared_ptr<arrow::Array> array(std::shared_ptr<arrow::RecordBatch>& batch,
                                      const Schema::Column& field);

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
  std::unique_ptr<Slice> slice_;
};

/******************************************************************************/
template <Type T>
Query::Result<Query::value_t> Executor::Impl::ArrayValueHelper::operator()()
{
  std::shared_ptr<arrow::Array> a(impl_.array(batch_, field_));

  if (row_index_ < 0 || row_index_ >= a->length() || a->IsNull(row_index_)) {
    return Query::Result<Query::value_t>::skip();
  }

  auto casted = std::static_pointer_cast<typename type_traits<T>::array_type>(a);
  auto value = Decoders::unsafe_array_value<T>(casted, row_index_);
  return Query::Result<Query::value_t>(value);
}

/******************************************************************************/
std::vector<bool> Executor::Impl::filter(const Planner::Plan& plan,
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
    Query::Result<bool> res =
        plan.query.eval(std::bind(get_value, i, std::placeholders::_1));
    want_rows[i] = !res.is(false);
  }

  return want_rows;
}

/******************************************************************************/
void Executor::Impl::project(std::shared_ptr<arrow::RecordBatch>& batch)
{
  for (const auto& field : slice_->fields()) {
    slice_->append(field, array(batch, field));
  }
}

/******************************************************************************/
std::shared_ptr<arrow::Array>
Executor::Impl::array(std::shared_ptr<arrow::RecordBatch>& batch,
                      const Schema::Column& field)
{
  if (field.first->is_root()) {
    return batch->column(field.second->absolute_index());
  } else {
    std::shared_ptr<arrow::Array> col(batch->column(field.first->index()));

    if (col && col->type_id() == arrow::Type::STRUCT) {
      auto sa = std::static_pointer_cast<arrow::StructArray>(col);
      return sa->field(field.second->relative_index());
    } else {
      throw ParquetError("column not in batch: " + field.first->path(*field.second));
    }
  }
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
std::unique_ptr<Executor::Slice> Executor::execute(const Planner::Plan& plan)
{
  impl_->slice_ = std::unique_ptr<Slice>(new Slice(impl_->projection_.get()));
  std::map<Schema::Group::index_type, std::vector<Planner::Range>> ranges;

  for (const auto& range : plan.ranges) {
    auto it = ranges.find(range.row_group);

    if (it != ranges.end()) {
      it->second.push_back(range);
    } else {
      ranges[range.row_group] = {range};
    }
  }

  // Reduce a batch to just those rows that were selected by the query
  // and then capture the projected columns.
  auto project_wanted = [&](std::size_t i, std::size_t j,
                            std::shared_ptr<arrow::RecordBatch>& batch) -> void {
    int64_t offset = i;
    int64_t length = j - i + 1;
    if (length <= 0) return;

    auto sliced = batch->Slice(offset, length);
    impl_->project(sliced);
  };

  for (const auto& row_group : ranges) {
    int64_t row_group_start = 0;

    auto batch_reader =
        impl_->check("invalid batch reader",
                     impl_->reader_->GetRecordBatchReader({row_group.first}));

    for (auto batch_r : *batch_reader) {
      auto batch = impl_->check("invalid record batch", batch_r);
      int64_t rows = batch->num_rows();

      for (const auto& range : row_group.second) {
        // Is this batch within this range?
        if (range.offset + range.length <= row_group_start ||
            range.offset >= row_group_start + rows) {
          continue;
        }

        int64_t offset = std::max(range.offset - row_group_start, int64_t{});
        int64_t length = std::min(range.length - row_group_start, rows);

        auto sliced = batch->Slice(offset, length);
        auto want_rows = impl_->filter(plan, sliced);
        Algorithm::spans(want_rows, project_wanted, sliced);
      }

      row_group_start += batch->num_rows();
    }
  }

  return std::move(impl_->slice_);
}

} // namespace MzPeak::Util
