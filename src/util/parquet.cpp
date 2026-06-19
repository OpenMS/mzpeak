/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <arrow/array.h>
#include <arrow/record_batch.h>
#include <arrow/util/key_value_metadata.h>
#include <boost/json.hpp>
#include <charconv>
#include <memory>
#include <parquet/api/reader.h>
#include <parquet/arrow/reader.h>
#include <ranges>

#include "mzpeak/exception.h"
#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/entity_type.h"
#include "mzpeak/util/arrow.h"
#include "mzpeak/util/parquet.h"

namespace MzPeak::Util {

/******************************************************************************/
std::optional<std::string> get_kv_string(Parquet::file_metadata_t& fmd,
                                         const std::string& key)
{
  auto result(fmd->key_value_metadata()->Get(key));

  if (result.ok()) {
    return result.ValueOrDie();
  } else {
    return {};
  }
}

/******************************************************************************/
std::optional<std::size_t> get_kv_uint(Parquet::file_metadata_t& fmd,
                                       const std::string& key)
{
  return get_kv_string(fmd, key).and_then(
      [](const std::string& s) -> std::optional<std::size_t> {
        std::size_t r{};
        auto [ptr, ec]{std::from_chars(s.data(), s.data() + s.size(), r)};

        if (ec == std::errc()) {
          return r;
        } else {
          return std::nullopt;
        }
      });
}

/******************************************************************************/
std::shared_ptr<Schema::ArrayIndex> parse_array_index(const std::string& str,
                                                      Schema::EntityType entity_type)
{
  namespace json = boost::json;

  boost::system::error_code ec;
  json::value v(json::parse(str));
  if (ec) throw MzPeak::JsonError(ec.message());

  if (v.is_object()) {
    return std::make_shared<Schema::ArrayIndex>(entity_type, v.as_object());
  } else {
    throw JsonError("array_index should be a JSON object");
  }
}

/******************************************************************************/
struct Parquet::Impl {
  Impl(std::unique_ptr<File> data, Schema::File file)
      : file_(std::move(file))
      , arrow_(std::make_unique<Arrow>(std::move(data)))
      , structs_(std::make_shared<StructMap>())
  {
    auto raf = arrow_->reader();

    auto reader_builder = parquet::arrow::FileReaderBuilder();
    auto status = reader_builder.Open(std::move(raf));

    if (!status.ok()) {
      std::string msg("while opening file: " + file.file_name + ": ");
      throw ParquetError(msg + status.ToString());
    }

    std::unique_ptr<parquet::arrow::FileReader> reader;
    status = reader_builder.Build(&reader);

    if (!status.ok()) {
      std::string msg("while reading file: " + file.file_name + ": ");
      throw ParquetError(msg + status.ToString());
    }

    reader_ = std::move(reader);
    parse_schema();
  }

  ~Impl() = default;

  void error(const std::string& error)
  {
    std::string msg("file accessing " + file_.file_name + ": " + error);
    throw ParquetError(msg);
  }

  /// Load the schema.
  void parse_schema();

  // Get the array index JSON and number of entities.
  std::pair<std::string, std::size_t> array_index(Parquet::file_metadata_t&);

  Schema::File file_;
  std::unique_ptr<Arrow> arrow_;
  std::shared_ptr<parquet::arrow::FileReader> reader_;
  std::shared_ptr<StructMap> structs_;
};

/******************************************************************************/
void Parquet::Impl::parse_schema()
{
  auto fmd = reader_->parquet_reader()->metadata();
  auto root = fmd->schema()->group_node();
  int32_t offset = 0;

  for (int32_t i : std::views::iota(0, root->field_count())) {
    auto node = root->field(i);

    // TODO: Should we emit a warning if there is a top-level
    // primitive column?
    if (node->is_group()) {
      std::shared_ptr<parquet::schema::GroupNode> group =
          std::static_pointer_cast<parquet::schema::GroupNode>(node);

      std::shared_ptr<Struct> s = std::make_shared<Struct>(*group, i, offset);
      (*structs_)[s->name()] = s;
      offset += group->field_count();
    }
  }
}

/******************************************************************************/
std::pair<std::string, std::size_t>
Parquet::Impl::array_index(Parquet::file_metadata_t& fmd)
{
  Schema::EntityType entity_type(file_.entity_type);

  std::string num_key(Schema::entity_type_to_string(entity_type) + "_count");
  std::optional<std::size_t> num_entities(get_kv_uint(fmd, num_key));

  std::string index_key(Schema::entity_type_to_string(entity_type) + "_array_index");
  auto index_str(get_kv_string(fmd, index_key));
  if (!index_str.has_value()) throw ParquetError("missing array_index");

  return std::make_pair<>(*index_str, num_entities.value_or(0));
}

/******************************************************************************/
Parquet::Parquet(std::unique_ptr<File> data, Schema::File file)
    : impl_(std::make_unique<Impl>(std::move(data), std::move(file)))
{
}

/******************************************************************************/
Parquet::~Parquet() = default;

/******************************************************************************/
const Schema::File& Parquet::index_file() const { return impl_->file_; }

/******************************************************************************/
const std::shared_ptr<StructMap>& Parquet::structs() const
{
  return impl_->structs_;
}

/******************************************************************************/
std::optional<Query::destination_t>
Parquet::field(const std::string_view& struct_name,
               const std::string_view& field_name) const
{
  auto struct_ptr = impl_->structs_->find(std::string{struct_name});
  if (struct_ptr == impl_->structs_->end()) return {};

  auto field_ptr = struct_ptr->second->field(field_name);
  if (!field_ptr.has_value()) return {};

  return std::make_pair(struct_ptr->second, field_ptr.value());
}

/******************************************************************************/
Parquet::file_metadata_t Parquet::file_metadata() const
{
  return impl_->reader_->parquet_reader()->metadata();
}

/******************************************************************************/
std::string Parquet::array_index_json() const
{
  file_metadata_t fmd(file_metadata());
  return impl_->array_index(fmd).first;
}

/******************************************************************************/
std::shared_ptr<Schema::ArrayIndex> Parquet::array_index() const
{
  file_metadata_t fmd(file_metadata());
  auto [index_str, num_entities] = impl_->array_index(fmd);

  std::shared_ptr<Schema::ArrayIndex> ai =
      parse_array_index(index_str, impl_->file_.entity_type);
  ai->num_entities(num_entities);

  const parquet::SchemaDescriptor* schema(fmd->schema());
  Schema::ArrayIndex::ColumnMap index_map;

  for (auto& column : ai->columns()) {
    int column_index = schema->ColumnIndex(column.path);

    if (column_index < 0) {
      std::string msg("while reading index from " + impl_->file_.file_name);
      msg += ": column index out of bounds for column: " + column.path;
      throw ParquetError(msg);
    }

    index_map[column.path] = column_index;
  }

  return ai;
}

/******************************************************************************/
parquet::arrow::FileReader& Parquet::reader() const { return *impl_->reader_; }

/******************************************************************************/
Planner Parquet::planner(const Query& q) { return Planner(*impl_->reader_, q); }

/******************************************************************************/
Executor Parquet::executor(const Executor::Projection& p)
{
  return Executor(impl_->reader_, p);
}

} // namespace MzPeak::Util
