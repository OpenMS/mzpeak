/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <arrow/record_batch.h>
#include <memory>
#include <parquet/arrow/reader.h>

#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/util/data_arrays.h"
#include "mzpeak/util/parquet_types.h"

namespace MzPeak::Util {

namespace psi = Schema::PSI;
using namespace std::placeholders;

/******************************************************************************/
using rec_batch_t = std::shared_ptr<arrow::RecordBatch>;

/******************************************************************************/
/// Try to get an array out of a batch.
std::shared_ptr<arrow::Array>
array_from_batch(arrow::RecordBatch& batch, const Schema::ArrayIndex& index,
                 const Schema::ArrayIndex::Array& array) {
  std::shared_ptr<arrow::Array> col;
  std::optional<int> i = index.column_index(array);

  // Could it be in a struct?
  if (col = batch.GetColumnByName(index.prefix()); col && i) {
    if (col->type_id() == arrow::Type::STRUCT) {
      auto sa = std::static_pointer_cast<arrow::StructArray>(col);
      return sa->field(*i);
    }
  }

  // What if I can access it it directly?
  if (col = batch.GetColumnByName(array.path); col) {
    return col;
  }

  throw ParquetError("array not in batch: " + array.path);
}

/******************************************************************************/
struct Batch {
  Batch(std::shared_ptr<arrow::RecordBatch> batch, Schema::ArrayIndex& index)
      : batch_(std::move(batch)), array_index_(index), slice_offset_(0),
        slice_length_(batch_->num_rows()) {};

  // Return an array with caching.
  std::shared_ptr<arrow::Array> cached_array(const Schema::ArrayIndex::Array& array);

  // Returned a sliced batch.
  std::shared_ptr<arrow::RecordBatch> slice_batch(const Query&);

  // Adjust the slice with a query.
  void query_batch(const Query&);

  std::shared_ptr<arrow::RecordBatch> batch_;
  Schema::ArrayIndex& array_index_;
  std::map<int, std::shared_ptr<arrow::Array>> cache_;
  int64_t slice_offset_;
  int64_t slice_length_;
};

/******************************************************************************/
struct DataArrays::Impl {
  Impl(std::unique_ptr<Util::Parquet> parquet)
      : parquet_(std::move(parquet)), array_index_(parquet_->array_index()) {};

  std::unique_ptr<Util::Parquet> parquet_;
  Schema::ArrayIndex array_index_;
};

/******************************************************************************/
std::shared_ptr<arrow::Array>
Batch::cached_array(const Schema::ArrayIndex::Array& array) {
  std::optional<int> index(array_index_.column_index(array));

  if (!index.has_value()) {
    std::string msg("array doesn't appear in the schema: " + array.path);
    throw ParquetError(msg);
  }

  auto it = cache_.find(*index);

  if (it != cache_.end()) {
    return it->second;
  } else {
    auto v = array_from_batch(*batch_, array_index_, array);
    cache_[*index] = v;
    return v;
  }
}

/******************************************************************************/
std::shared_ptr<arrow::RecordBatch> Batch::slice_batch(const Query& query) {
  query_batch(query);

  if (slice_offset_ == 0 && slice_length_ == batch_->num_rows()) {
    return batch_;
  } else {
    return batch_->Slice(slice_offset_, slice_length_);
  }
}

/******************************************************************************/
struct ArrayValueHelper {
  template <psi::DataType T>
  std::optional<Query::value_t>
  operator()(const Schema::ArrayIndex::Array& array) const {
    auto raw = batch_.cached_array(array);
    auto data = Util::parquet_array_cast<T>(raw);
    return data->Value(i_);
  }

  Batch& batch_;
  int64_t i_;
};

template <> // Specialized since we don't support ASCII types.
std::optional<Query::value_t> ArrayValueHelper::operator()<psi::DataType::ASCII>(
    const Schema::ArrayIndex::Array& _) const {
  return {};
}

/******************************************************************************/
void Batch::query_batch(const Query& query) {
  auto get_value =
      [&](ArrayValueHelper& helper,
          const Schema::ArrayIndex::Array& array) -> std::optional<Query::value_t> {
    return psi::dispatch(array.data_type, helper, array);
  };

  { // Find the first "row" that matches the query.
    ArrayValueHelper forward{*this, slice_offset_};

    for (; forward.i_ < slice_length_; ++forward.i_) {
      if (query.eval(std::bind(get_value, std::ref(forward), _1))) {
        break;
      }
    }

    if (forward.i_ == slice_length_) {
      // No matches.
      slice_offset_ = 0;
      slice_length_ = 0;
      return;
    } else {
      slice_offset_ = forward.i_;
    }
  }

  { // Find the last "row" that matches the query.
    if (slice_length_ <= 0 || slice_offset_ >= slice_length_) return;
    ArrayValueHelper backward{*this, slice_length_ - 1};

    for (; backward.i_ > slice_offset_; --backward.i_) {
      if (query.eval(std::bind(get_value, std::ref(backward), _1))) {
        break;
      }
    }

    if (backward.i_ == slice_offset_) {
      // No matches.
      slice_offset_ = 0;
      slice_length_ = 0;
      return;
    } else {
      slice_length_ = backward.i_ + 1;
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
DataArrays::read_arrays(const Query& query,
                        const std::vector<Schema::ArrayIndex::Array>& arrays) {
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
      auto helper = Batch(batch_r.ValueOrDie(), impl_->array_index_);
      batch = helper.slice_batch(query);
    }

    for (auto& array : arrays) {
      std::optional<int> index(impl_->array_index_.column_index(array));

      std::optional<std::shared_ptr<arrow::Array>> data =
          array_from_batch(*batch, impl_->array_index_, array);

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
