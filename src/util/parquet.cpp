/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <arrow/array.h>
#include <arrow/dataset/scanner.h>
#include <arrow/record_batch.h>
#include <arrow/util/key_value_metadata.h>
#include <boost/json.hpp>
#include <charconv>
#include <memory>
#include <parquet/api/reader.h>
#include <parquet/arrow/reader.h>
#include <ranges>
#include <utility>

#include "mzpeak/exception.h"
#include "mzpeak/util/arrow.h"
#include "mzpeak/util/parquet.h"

namespace MzPeak::Util {

/******************************************************************************/
std::optional<std::string> get_kv_string(const Parquet::file_metadata_t& fmd,
                                         std::string_view key)
{
  auto result(fmd->key_value_metadata()->Get(key));

  if (result.ok()) {
    return result.ValueOrDie();
  } else {
    return {};
  }
}

/******************************************************************************/
std::optional<std::size_t> get_kv_uint(const Parquet::file_metadata_t& fmd,
                                       std::string_view key)
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
struct Parquet::Impl {
  Impl(std::unique_ptr<IO::File> data, Schema::File file)
      : file_(std::move(file))
      , arrow_(std::make_unique<Arrow>(std::move(data)))
      , reader_(nullptr)
      , groups_(std::make_shared<Schema::GroupMap>())
  {
    auto raf = arrow_->reader();

    auto reader_builder = parquet::arrow::FileReaderBuilder();
    auto status = reader_builder.Open(std::move(raf));

    if (!status.ok()) {
      std::string msg("while opening file: " + file_.file_name() + ": ");
      throw ParquetError(msg + status.ToString());
    }

    std::unique_ptr<parquet::arrow::FileReader> reader;
    status = reader_builder.Build(&reader);

    if (!status.ok()) {
      std::string msg("while reading file: " + file_.file_name() + ": ");
      throw ParquetError(msg + status.ToString());
    }

    reader_ = std::move(reader);
    parse_schema();
  }

  ~Impl() = default;

  void error(const std::string& error)
  {
    std::string msg("file accessing " + file_.file_name() + ": " + error);
    throw ParquetError(msg);
  }

  /// Helper to check a result and throw an error if necessary.
  template <typename T> T check_result(arrow::Result<T> r)
  {
    if (r.ok()) {
      return std::move(r.ValueOrDie());
    } else {
      error(r.status().ToString());
      std::unreachable();
    }
  }

  /// Helper to assert an arrow status.
  void check_status(const arrow::Status& s)
  {
    if (!s.ok()) {
      error(s.ToString());
    }
  }

  /// Load the schema.
  void parse_schema();

  /// Return the array associated with the given column.
  std::shared_ptr<arrow::Array> array(std::shared_ptr<arrow::RecordBatch>&,
                                      const Schema::Column&);

  Schema::File file_;
  std::unique_ptr<Arrow> arrow_;
  std::shared_ptr<parquet::arrow::FileReader> reader_;
  std::shared_ptr<Schema::GroupMap> groups_;
};

/******************************************************************************/
void Parquet::Impl::parse_schema()
{
  auto fmd = reader_->parquet_reader()->metadata();
  auto root = fmd->schema()->group_node();
  int32_t offset = 0;

  std::shared_ptr<Schema::Group> root_group =
      std::make_shared<Schema::Group>(*root, file_);

  if (!root_group->fields().empty()) {
    (*groups_)[root_group->name()] = root_group;
  }

  for (int32_t i : std::views::iota(0, root->field_count())) {
    auto node = root->field(i);

    if (node->is_group() && !node->logical_type()->is_list()) {
      std::shared_ptr<parquet::schema::GroupNode> group =
          std::static_pointer_cast<parquet::schema::GroupNode>(node);

      std::shared_ptr<Schema::Group> s =
          std::make_shared<Schema::Group>(*group, file_, i, offset);
      (*groups_)[s->name()] = s;
      offset += group->field_count();
    }
  }
}

/******************************************************************************/
std::shared_ptr<arrow::Array>
Parquet::Impl::array(std::shared_ptr<arrow::RecordBatch>& batch,
                     const Schema::Column& column)
{
  if (column.first->is_root()) {
    return batch->column(column.second->absolute_index());
  } else {
    std::shared_ptr<arrow::Array> ary(batch->column(column.first->index()));

    if (ary && ary->type_id() == arrow::Type::STRUCT) {
      auto sa = std::static_pointer_cast<arrow::StructArray>(ary);
      return sa->field(column.second->relative_index());
    } else {
      error("column not in batch: " + column.first->path(*column.second));
      std::unreachable();
    }
  }
}

/******************************************************************************/
Parquet::Parquet(std::unique_ptr<IO::File> data, Schema::File file)
    : impl_(std::make_unique<Impl>(std::move(data), std::move(file)))
{
}

/******************************************************************************/
Parquet::~Parquet() = default;

/******************************************************************************/
const Schema::File& Parquet::index_file() const { return impl_->file_; }

/******************************************************************************/
const std::shared_ptr<Schema::GroupMap>& Parquet::groups() const
{
  return impl_->groups_;
}

/******************************************************************************/
std::optional<Schema::Column> Parquet::field(std::string_view group_name,
                                             std::string_view field_name) const
{
  auto group_ptr = impl_->groups_->find(std::string{group_name});
  if (group_ptr == impl_->groups_->end()) return {};

  auto field_ptr = group_ptr->second->field(std::move(field_name));
  if (!field_ptr.has_value()) return {};

  return std::make_pair(group_ptr->second, field_ptr.value());
}

/******************************************************************************/
Parquet::file_metadata_t Parquet::file_metadata() const
{
  return impl_->reader_->parquet_reader()->metadata();
}

/******************************************************************************/
std::optional<std::string> Parquet::kv_string(const file_metadata_t& fmd,
                                              std::string_view key) const
{
  return get_kv_string(fmd, key);
}

/******************************************************************************/
std::optional<std::size_t> Parquet::kv_size_t(const file_metadata_t& fmd,
                                              std::string_view key) const
{
  return get_kv_uint(fmd, key);
}

/******************************************************************************/
parquet::arrow::FileReader& Parquet::reader() const { return *impl_->reader_; }

/******************************************************************************/
void Parquet::read(const Projection& proj, const Filter& filter, Reader fn) const
{
  std::shared_ptr<arrow::RecordBatchReader> batch =
      impl_->check_result(impl_->reader_->GetRecordBatchReader());

  std::shared_ptr<arrow::dataset::ScannerBuilder> builder =
      arrow::dataset::ScannerBuilder::FromRecordBatchReader(batch);

  std::vector<std::string> columns =
      proj.get() |
      std::views::transform([](auto& col) { return col.first->path(*col.second); }) |
      std::ranges::to<std::vector>();

  impl_->check_status(builder->Project(columns));
  // impl_->check_status(builder->UseThreads());

  auto expr = filter.expression();

  if (expr.has_value()) {
    impl_->check_status(builder->Filter(expr.value()));
  }

  std::shared_ptr<arrow::dataset::Scanner> scanner =
      impl_->check_result(builder->Finish());

  std::shared_ptr<arrow::RecordBatchReader> reader =
      impl_->check_result(scanner->ToRecordBatchReader());

  for (const auto& batch_r : *reader) {
    auto batch = Batch(impl_->check_result(batch_r));
    if (!fn(batch)) break;
  }
}

/******************************************************************************/
void Parquet::read(const Projection& proj, Reader fn) const
{
  read(proj, {}, std::move(fn));
}

/******************************************************************************/
Planner Parquet::planner(const Query& q) { return Planner(*impl_->reader_, q); }

/******************************************************************************/
Executor Parquet::executor(const Projection& p)
{
  return Executor(impl_->reader_, p);
}

} // namespace MzPeak::Util
