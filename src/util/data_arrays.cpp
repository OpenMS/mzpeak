/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <algorithm>
#include <arrow/record_batch.h>
#include <iterator>
#include <memory>
#include <parquet/arrow/reader.h>

#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/util/data_arrays.h"
#include "mzpeak/util/parquet_types.h"

namespace MzPeak::Util {

/******************************************************************************/
using rec_batch_t = std::shared_ptr<arrow::RecordBatch>;

/******************************************************************************/
struct DataArrays::Impl {
  Impl(std::unique_ptr<Util::Parquet> parquet)
      : parquet_(std::move(parquet)), array_index_(parquet_->array_index()) {};

  struct Batch {
    Batch(std::shared_ptr<arrow::RecordBatch> batch)
        : batch_(std::move(batch)), slice_offset_(0),
          slice_length_(batch_->num_rows()) {};

    std::shared_ptr<arrow::RecordBatch> batch_;
    std::map<int, std::shared_ptr<arrow::Array>> cache_;
    int64_t slice_offset_;
    int64_t slice_length_;
  };

  /// Try to get an array out of a batch.
  std::shared_ptr<arrow::Array>
  array_from_batch(arrow::RecordBatch&, const Schema::ArrayIndex::Array&) const;

  // Return an array with caching.
  std::shared_ptr<arrow::Array> cached_array(Batch& batch,
                                             const DataArrays::Array& array);

  // Returned a sliced batch.
  std::shared_ptr<arrow::RecordBatch> slice_batch(Batch&, const Query&);

  // Adjust the slice with a query.
  template <Schema::PSI::DataType T>
  void query_batch(Batch&, const Query::Predicate<T>&);

  std::unique_ptr<Util::Parquet> parquet_;
  Schema::ArrayIndex array_index_;
};

/******************************************************************************/
std::shared_ptr<arrow::Array>
DataArrays::Impl::array_from_batch(arrow::RecordBatch& batch,
                                   const Schema::ArrayIndex::Array& array) const {
  std::shared_ptr<arrow::Array> col;
  std::optional<int> index = array_index_.column_index(array);

  // Could it be in a struct?
  if (col = batch.GetColumnByName(array_index_.prefix()); col && index) {
    if (col->type_id() == arrow::Type::STRUCT) {
      auto sa = std::static_pointer_cast<arrow::StructArray>(col);
      return sa->field(*index);
    }
  }

  // What if I can access it it directly?
  if (col = batch.GetColumnByName(array.path); col) {
    return col;
  }

  throw ParquetError("array not in batch: " + array.path);
}

/******************************************************************************/
std::shared_ptr<arrow::Array>
DataArrays::Impl::cached_array(Batch& batch, const DataArrays::Array& array) {
  std::optional<int> index(array_index_.column_index(array));

  if (!index.has_value()) {
    std::string msg("array doesn't appear in the schema: " + array.path);
    throw ParquetError(msg);
  }

  auto it = batch.cache_.find(*index);

  if (it != batch.cache_.end()) {
    return it->second;
  } else {
    auto v = array_from_batch(*batch.batch_, array);
    batch.cache_[*index] = v;
    return v;
  }
}

/******************************************************************************/
std::shared_ptr<arrow::RecordBatch>
DataArrays::Impl::slice_batch(Batch& batch, const Query& query) {
  for (auto& pred : query.predicates()) {
    std::visit(
        [&](auto&& typed_pred) {
          using enum Schema::PSI::DataType;
          using T = std::decay_t<decltype(typed_pred)>;

          if constexpr (std::is_same_v<T, Query::Predicate<Int32>>) {
            query_batch(batch, typed_pred);
          } else if constexpr (std::is_same_v<T, Query::Predicate<Float32>>) {
            query_batch(batch, typed_pred);
          } else if constexpr (std::is_same_v<T, Query::Predicate<Int64>>) {
            query_batch(batch, typed_pred);
          } else if constexpr (std::is_same_v<T, Query::Predicate<Float64>>) {
            query_batch(batch, typed_pred);
          } else {
            static_assert(false, "failed to detect predicate type!");
          }
        },
        pred);
  }

  if (batch.slice_offset_ == 0 && batch.slice_length_ == batch.batch_->num_rows()) {
    return batch.batch_;
  } else {
    return batch.batch_->Slice(batch.slice_offset_, batch.slice_length_);
  }
}

/******************************************************************************/
template <Schema::PSI::DataType T>
void DataArrays::Impl::query_batch(Batch& batch, const Query::Predicate<T>& pred) {
  using value_type = typename Schema::PSI::data_type_traits<T>::value_type;

  auto raw = cached_array(batch, pred.array());
  auto data = Util::parquet_array_cast<T>(raw);

  auto match = [pred](std::optional<value_type> v) -> bool {
    // Can't compare null values.
    return v.has_value() && pred.match(*v);
  };

  // N.B. If we are working with a sorted array then we can narrow the
  // batch by trimming the head and tail of the array.  Otherwise we
  // keep the batch intact and filter later.
  if (pred.array().sorting_rank.has_value() &&
      pred.array().sorting_rank.value() == 0) {

    auto begin = data->begin() + batch.slice_offset_;
    auto end = data->begin() + batch.slice_length_;
    auto first = std::find_if(begin, end, match);

    if (first == end) {
      // We don't even want this batch!
      batch.slice_length_ = 0;
      return;
    } else {
      batch.slice_offset_ += std::distance(begin, first);
      begin = first;
    }

    auto begin_r = std::make_reverse_iterator(end);
    auto end_r = std::make_reverse_iterator(begin);
    auto last = std::find_if(begin_r, end_r, match);

    if (last != end_r) {
      batch.slice_length_ -= std::distance(begin_r, last);
    }
  }
}

/******************************************************************************/
DataArrays::DataArrays(std::unique_ptr<Util::Parquet> parquet)
    : impl_(std::make_unique<Impl>(std::move(parquet))) {}

/******************************************************************************/
DataArrays::~DataArrays() = default;

/******************************************************************************/
const Schema::ArrayIndex& DataArrays::array_index() const {
  return impl_->array_index_;
}

/******************************************************************************/
std::unique_ptr<DataArrays::ArrayMap>
DataArrays::read_arrays(const Query& query, const std::vector<Array>& arrays) {
  std::unique_ptr<ArrayMap> map = std::make_unique<ArrayMap>();

  std::vector<int> indices(impl_->parquet_->find_row_groups(query));
  auto batch_reader_res = impl_->parquet_->reader().GetRecordBatchReader(indices);

  if (!batch_reader_res.ok()) {
    std::string msg("while creating a batch reader: ");
    throw ParquetError(msg + batch_reader_res.status().ToString());
  }

  std::unique_ptr<arrow::RecordBatchReader> batch_reader =
      std::move(batch_reader_res.ValueOrDie());

  for (auto batch_r : *batch_reader) {

    if (!batch_r.ok()) {
      std::string msg("while iterating over batches: ");
      throw ParquetError(msg + batch_r.status().ToString());
    }

    std::shared_ptr<arrow::RecordBatch> batch;

    {
      auto helper = Impl::Batch(batch_r.ValueOrDie());
      batch = impl_->slice_batch(helper, query);
    }

    for (auto& array : arrays) {
      std::optional<int> index(impl_->array_index_.column_index(array));

      std::optional<std::shared_ptr<arrow::Array>> data =
          impl_->array_from_batch(*batch, array);

      if (index.has_value() && data.has_value()) {
        auto existing = map->find(*index);

        if (existing != map->end()) {
          existing->second->push_back(*data);
        } else {
          std::shared_ptr<ArrayVector> vec = std::make_shared<ArrayVector>();
          vec->push_back(*data);
          (*map)[*index] = vec;
        }
      }
    }
  }

  return map;
}

} // namespace MzPeak::Util
