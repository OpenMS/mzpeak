/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <arrow/util/key_value_metadata.h>
#include <boost/json.hpp>
#include <charconv>
#include <memory>
#include <parquet/api/reader.h>
#include <parquet/arrow/reader.h>

#include "mzpeak/exception.h"
#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/entity_type.h"
#include "mzpeak/util/arrow.h"
#include "mzpeak/util/parquet.h"

namespace MzPeak::Util {

/******************************************************************************/
std::optional<std::string> get_kv_string(Parquet::file_metadata_t& fmd,
                                         const std::string& key) {
  auto result(fmd->key_value_metadata()->Get(key));

  if (result.ok()) {
    return result.ValueOrDie();
  } else {
    return {};
  }
}

/******************************************************************************/
std::optional<std::size_t> get_kv_uint(Parquet::file_metadata_t& fmd,
                                       const std::string& key) {
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
Schema::ArrayIndex parse_array_index(const std::optional<std::string>& str,
                                     Schema::EntityType entity_type) {
  namespace json = boost::json;
  if (!str.has_value()) throw ParquetError("missing array_index");

  boost::system::error_code ec;
  json::value v(json::parse(*str));
  if (ec) throw MzPeak::JsonError(ec.message());

  if (v.is_object()) {
    return Schema::ArrayIndex(entity_type, v.as_object());
  } else {
    throw JsonError("array_index should be a JSON object");
  }
}

/******************************************************************************/
struct Parquet::Impl {
  Impl(std::unique_ptr<File::Readable> data, Schema::File file)
      : file_(std::move(file)), arrow_(std::make_unique<Arrow>(std::move(data))) {
    auto raf = arrow_->reader();

    auto reader_builder = parquet::arrow::FileReaderBuilder();
    auto status = reader_builder.Open(std::move(raf));
    if (!status.ok()) throw ParquetError(status.ToString());

    status = reader_builder.Build(&reader_);
    if (!status.ok()) throw ParquetError(status.ToString());
  };

  ~Impl() = default;

  Schema::File file_;
  std::unique_ptr<Arrow> arrow_;
  std::unique_ptr<parquet::arrow::FileReader> reader_;
};

/******************************************************************************/
Parquet::Parquet(std::unique_ptr<File::Readable> data, Schema::File file)
    : impl_(std::make_unique<Impl>(std::move(data), std::move(file))) {}

/******************************************************************************/
Parquet::~Parquet() = default;

/******************************************************************************/
const Schema::File& Parquet::index_file() const { return impl_->file_; }

/******************************************************************************/
Parquet::file_metadata_t Parquet::file_metadata() const {
  return impl_->reader_->parquet_reader()->metadata();
}

/******************************************************************************/
Util::RowGroupMetadataProxy Parquet::rg_metadata() const {
  return Util::RowGroupMetadataProxy(file_metadata());
}

/******************************************************************************/
Schema::ArrayIndex Parquet::array_index() const {
  Schema::EntityType entity_type(impl_->file_.entity_type);
  file_metadata_t fmd(file_metadata());

  std::string num_key(Schema::entity_type_to_string(entity_type) + "_count");
  std::optional<std::size_t> num_entities(get_kv_uint(fmd, num_key));

  std::string index_key(Schema::entity_type_to_string(entity_type) + "_array_index");
  auto index_str(get_kv_string(fmd, index_key));

  Schema::ArrayIndex ai(parse_array_index(index_str, impl_->file_.entity_type));
  ai.num_entities(num_entities);

  return ai;
}

} // namespace MzPeak::Util
