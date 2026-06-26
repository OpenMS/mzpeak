/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <arrow/record_batch.h>
#include <memory>
#include <parquet/arrow/reader.h>

#include "mzpeak/data/array_index.h"
#include "mzpeak/data/signals.h"
#include "mzpeak/util/executor.h"
#include "mzpeak/util/planner.h"
#include "mzpeak/util/projection.h"

namespace MzPeak::Data {

/******************************************************************************/
struct Signals::Impl {
  Impl(std::unique_ptr<Util::Parquet> parquet)
      : parquet_(std::move(parquet))
  {
    array_index_ = parse_array_index();
  }

  std::shared_ptr<ArrayIndex> parse_array_index() const;

  std::unique_ptr<Util::Parquet> parquet_;
  std::shared_ptr<ArrayIndex> array_index_;
};

/******************************************************************************/
std::shared_ptr<ArrayIndex> Signals::Impl::parse_array_index() const
{
  Util::Parquet::file_metadata_t fmd(parquet_->file_metadata());
  EntityType entity_type = parquet_->index_file().entity_type;

  std::string num_key(Schema::entity_type_to_string(entity_type) + "_count");
  std::optional<std::size_t> num_entities(parquet_->kv_size_t(fmd, num_key));

  std::string index_key(Schema::entity_type_to_string(entity_type) + "_array_index");
  auto index_str(parquet_->kv_string(fmd, index_key));
  if (!index_str.has_value()) throw ParquetError("missing array_index");

  namespace json = boost::json;
  boost::system::error_code ec;
  json::value v(json::parse(index_str.value()));
  if (ec) throw MzPeak::JsonError(ec.message());

  if (v.is_object()) {
    auto ai = std::make_shared<ArrayIndex>(entity_type, v.as_object());
    ai->num_entities(num_entities);
    return ai;
  } else {
    throw JsonError("array_index should be a JSON object");
  }
}

/******************************************************************************/
Signals::Signals(std::unique_ptr<Util::Parquet> parquet)
    : impl_(std::make_unique<Impl>(std::move(parquet)))
{
}

/******************************************************************************/
Signals::~Signals() = default;

/******************************************************************************/
const std::shared_ptr<ArrayIndex>& Signals::array_index() const
{
  return impl_->array_index_;
}

/******************************************************************************/
std::size_t Signals::record_count() const
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
std::optional<Schema::Column> Signals::field(const std::string_view& name) const
{
  return impl_->parquet_->field(impl_->array_index_->prefix(), name);
}

/******************************************************************************/
const std::shared_ptr<Schema::GroupMap>& Signals::groups() const
{
  return impl_->parquet_->groups();
}

/******************************************************************************/
Util::Query::Builder Signals::index() const
{
  auto entity_type = impl_->array_index_->entity_type();
  auto field_name = Schema::entity_type_to_string(entity_type) + "_index";
  auto index_field = field(field_name);

  if (!index_field.has_value()) {
    throw ParquetError("parquet file is missing the index column: " + field_name);
  }

  return Util::Query::Builder(index_field.value());
}

/******************************************************************************/
std::unique_ptr<Util::Slice>
Signals::select(const std::vector<Dimension>& projection, const Util::Query& query)
{
  Util::Projection columns;

  for (auto& dim : projection) {
    auto entries = impl_->array_index_->entries(dim);

    for (auto& entry : entries) {
      auto field =
          impl_->array_index_->entry_column(*impl_->parquet_->groups(), entry);
      if (!field.has_value()) {
        throw ParquetError("array entry not present in schema: " + entry.name);
      } else {
        columns.project(field.value());
      }
    }
  }

  Util::Planner planner = impl_->parquet_->planner(query);
  auto plan = planner.plan();

  Util::Executor executor = impl_->parquet_->executor(columns);
  return executor.execute(plan);
}

} // namespace MzPeak::Data
