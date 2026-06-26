/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <arrow/record_batch.h>
#include <memory>
#include <parquet/arrow/reader.h>

#include "mzpeak/data/arrays.h"
#include "mzpeak/data/encoding.h"
#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/util/parquet_types.h"

namespace MzPeak::Data {

namespace psi = Schema::PSI;
using namespace std::placeholders;

/******************************************************************************/
using rec_batch_t = std::shared_ptr<arrow::RecordBatch>;

/******************************************************************************/
/// Try to get an array out of a batch.
std::shared_ptr<arrow::Array> array_from_batch(arrow::RecordBatch& batch,
                                               const Util::Struct& index,
                                               const Util::Struct::Field& column)
{
  std::shared_ptr<arrow::Array> col(batch.column(index.index()));

  if (col && col->type_id() == arrow::Type::STRUCT) {
    auto sa = std::static_pointer_cast<arrow::StructArray>(col);
    return sa->field(column.relative_index());
  }

  throw ParquetError("column not in batch: " + column.name());
}

/******************************************************************************/
struct Batch {
  Batch(std::shared_ptr<arrow::RecordBatch> batch, const Util::Struct& index)
      : batch_(std::move(batch))
      , array_index_(index)
      , slice_offset_(0)
      , slice_length_(batch_->num_rows())
  {
  }

  // Return an array with caching.
  std::shared_ptr<arrow::Array> cached_array(const Util::Struct::Field& column);

  // Returned a sliced batch.
  std::shared_ptr<arrow::RecordBatch> slice_batch(const Query&);

  // Adjust the slice with a query.
  void query_batch(const Query&);

  std::shared_ptr<arrow::RecordBatch> batch_;
  const Util::Struct& array_index_;
  std::map<int, std::shared_ptr<arrow::Array>> cache_;
  int64_t slice_offset_;
  int64_t slice_length_;
};

/******************************************************************************/
struct Arrays::Impl {
  Impl(std::unique_ptr<Util::Parquet> parquet)
      : parquet_(std::move(parquet))
      , array_index_(parquet_->array_index())
  {
  }

  std::unique_ptr<Util::Parquet> parquet_;
  Schema::ArrayIndex array_index_;
};

/******************************************************************************/
std::shared_ptr<arrow::Array> Batch::cached_array(const Util::Struct::Field& column)
{
  auto it = cache_.find(column.relative_index());

  if (it != cache_.end()) {
    return it->second;
  } else {
    auto v = array_from_batch(*batch_, array_index_, column);
    cache_[column.relative_index()] = v;
    return v;
  }
}

/******************************************************************************/
std::shared_ptr<arrow::RecordBatch> Batch::slice_batch(const Query& query)
{
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
  Query::Result<Query::value_t> operator()(const Util::Struct::Field& column) const
  {
    auto raw = batch_.cached_array(column);
    auto data = Util::parquet_array_cast<T>(raw);
    return Query::Result<Query::value_t>(data->Value(i_));
  }

  Batch& batch_;
  int64_t i_;
};

template <> // Specialized since we don't support ASCII types.
Query::Result<Query::value_t> ArrayValueHelper::operator()<psi::DataType::ASCII>(
    const Util::Struct::Field& _) const
{
  return Query::Result<Query::value_t>::fail();
}

/******************************************************************************/
void Batch::query_batch(const Query& query)
{
  auto get_value =
      [&](ArrayValueHelper& helper,
          const Query::destination_t& dest) -> Query::Result<Query::value_t> {
    if (dest.second->data_type().has_value()) {
      return psi::dispatch(dest.second->data_type().value(), helper, *dest.second);
    } else {
      return Query::Result<Query::value_t>::fail();
    }
  };

  { // Find the first "row" that matches the query.
    ArrayValueHelper forward{*this, slice_offset_};

    for (; forward.i_ < slice_length_; ++forward.i_) {
      auto res = query.eval(std::bind(get_value, std::ref(forward), _1));
      if (res.has_value() && res.value()) {
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
      auto res = query.eval(std::bind(get_value, std::ref(backward), _1));
      if (res.has_value() && res.value()) {
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
Arrays::Arrays(std::unique_ptr<Util::Parquet> parquet)
    : impl_(std::make_unique<Impl>(std::move(parquet)))
{
}

/******************************************************************************/
Arrays::~Arrays() = default;

/******************************************************************************/
const Schema::ArrayIndex& Arrays::array_index() const { return impl_->array_index_; }

/******************************************************************************/
std::size_t Arrays::record_count() const
{
  auto ne(impl_->array_index_.num_entities());
  if (ne.has_value()) return *ne;

  // Fall back to scanning row-group statistics on the spectrum_index column.
  // The index is 0-based and contiguous, so COUNT = max_index + 1.
  const std::string prefix = impl_->array_index_.prefix();
  auto dest = impl_->parquet_->field(prefix, "spectrum_index");
  if (!dest.has_value()) {
    throw ParquetError("record_count: missing " + prefix + ".spectrum_index column");
  }
  const int col = dest->second->absolute_index();

  auto fmd = impl_->parquet_->file_metadata();
  if (fmd->num_row_groups() == 0) return 0;

  int64_t max_index = -1;
  for (int g = 0; g < fmd->num_row_groups(); ++g) {
    auto chunk = fmd->RowGroup(g)->ColumnChunk(col);
    if (!chunk->is_stats_set()) continue;
    auto stats = chunk->statistics();
    if (!stats || !stats->HasMinMax()) continue;
    auto typed = std::dynamic_pointer_cast<parquet::Int64Statistics>(stats);
    if (!typed) continue;
    max_index = std::max(max_index, typed->max());
  }

  if (max_index < 0) {
    throw ParquetError("record_count: no statistics for " + prefix +
                       ".spectrum_index");
  }
  return static_cast<std::size_t>(max_index) + 1;
}

/******************************************************************************/
std::optional<Query::destination_t> Arrays::field(const std::string_view& name) const
{
  return impl_->parquet_->field(impl_->array_index_.prefix(), name);
}

/******************************************************************************/
std::vector<Util::Struct::Field>
Arrays::columns_to_fields(const std::vector<Schema::ArrayIndex::Column>& cols) const
{
  auto structs = impl_->parquet_->structs();
  auto prefix_struct = structs.find(impl_->array_index_.prefix());
  if (prefix_struct == structs.end()) return {};

  std::vector<Util::Struct::Field> res;
  res.reserve(cols.size());

  for (const auto& col : cols) {
    std::string name = col.path.substr(impl_->array_index_.prefix().size() + 1);
    auto field = prefix_struct->second->field(name);

    if (field.has_value()) {
      res.push_back(*field.value());
    }
  }

  return res;
}

/******************************************************************************/
std::unique_ptr<array_map_type>
Arrays::read_arrays(const Query& query,

                    const std::vector<Util::Struct::Field>& columns)
{
  auto array_struct = impl_->parquet_->structs().find(impl_->array_index_.prefix());
  auto layout = *array_struct->second; // FIXME: Not check if this can be derefed

  std::unique_ptr<array_map_type> map = std::make_unique<array_map_type>();

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
      auto helper = Batch(batch_r.ValueOrDie(), layout);
      batch = helper.slice_batch(query);
    }

    for (auto& column : columns) {
      std::optional<std::shared_ptr<arrow::Array>> data =
          array_from_batch(*batch, layout, column);

      if (data.has_value()) {
        auto existing = map->find(column.relative_index());

        if (existing != map->end()) {
          existing->second->push_back(*data);
        } else {
          std::shared_ptr<raw_array_type> vec = std::make_shared<raw_array_type>();
          vec->push_back(*data);
          (*map)[column.relative_index()] = vec;
        }
      }
    }
  }

  return map;
}

} // namespace MzPeak::Data
