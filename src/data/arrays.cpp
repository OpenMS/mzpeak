/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <arrow/record_batch.h>
#include <memory>
#include <parquet/arrow/reader.h>

#include "mzpeak/data/arrays.h"
#include "mzpeak/schema/array_index.h"
#include "mzpeak/util/executor.h"
#include "mzpeak/util/planner.h"

namespace MzPeak::Data {

/******************************************************************************/
struct Arrays::Impl {
  Impl(std::unique_ptr<Util::Parquet> parquet)
      : parquet_(std::move(parquet))
      , array_index_(parquet_->array_index())
  {
  }

  std::unique_ptr<Util::Parquet> parquet_;
  std::shared_ptr<Schema::ArrayIndex> array_index_;
};

/******************************************************************************/
Arrays::Arrays(std::unique_ptr<Util::Parquet> parquet)
    : impl_(std::make_unique<Impl>(std::move(parquet)))
{
}

/******************************************************************************/
Arrays::~Arrays() = default;

/******************************************************************************/
const std::shared_ptr<Schema::ArrayIndex>& Arrays::array_index() const
{
  return impl_->array_index_;
}

/******************************************************************************/
std::size_t Arrays::record_count() const
{
  auto ne(impl_->array_index_->num_entities());
  if (ne.has_value()) return *ne;

  // The first column should be the index.
  //
  // FIXME: Is there a better way to do this?
  // const Schema::ArrayIndex::Column& index(impl_->array_index_.columns()[0]);
  // std::optional<int> col_idx(impl_->array_index_.column_index(index));
  // if (!col_idx.has_value()) throw ParquetError("missing column: " + index.path);
  //
  // std::optional<Util::Parquet::Stats> stats(
  //     impl_->parquet_->statistics(-1, *col_idx));
  //
  // if (stats.has_value()) {
  //   auto tptr(Util::parquet_statistics_cast<Schema::PSI::DataType::Int64>(
  //       *stats->column, *stats->stats));
  //
  //   return tptr->max();
  // }

  // FIXME: Should we scan the file at this point?
  throw ParquetError("no num_entities cache and no column statistics!");
}

/******************************************************************************/
std::optional<Query::destination_t> Arrays::field(const std::string_view& name) const
{
  return impl_->parquet_->field(impl_->array_index_->prefix(), name);
}

/******************************************************************************/
const std::shared_ptr<Util::StructMap>& Arrays::structs() const
{
  return impl_->parquet_->structs();
}

/******************************************************************************/
Query::Builder Arrays::index() const
{
  auto entity_type = impl_->array_index_->entity_type();
  auto field_name = Schema::entity_type_to_string(entity_type) + "_index";
  auto index_field = field(field_name);

  if (!index_field.has_value()) {
    throw ParquetError("parquet file is missing the index column: " + field_name);
  }

  return Query::Builder(index_field.value());
}

/******************************************************************************/
std::unique_ptr<Slice> Arrays::select(const std::vector<Dimension>& projection,
                                      const Query& query)
{
  std::vector<Util::Column> columns;

  for (auto& dim : projection) {
    auto entries = impl_->array_index_->columns(dim);

    for (auto& entry : entries) {
      auto field =
          impl_->array_index_->entry_column(*impl_->parquet_->structs(), entry);
      if (!field.has_value()) {
        throw ParquetError("array entry not present in schema: " + entry.name);
      } else {
        columns.push_back(field.value());
      }
    }
  }

  Util::Planner planner = impl_->parquet_->planner(query);
  auto plan = planner.plan();

  Util::Executor executor = impl_->parquet_->executor(columns);
  return executor.execute(plan);
}

} // namespace MzPeak::Data
