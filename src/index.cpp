/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/archive.h"
#include "mzpeak/exception.h"
#include "mzpeak/index.h"

#include <memory>

/*
 * Boost JSON:
 *   https://www.boost.org/doc/libs/latest/libs/json/doc/html/index.html
 */
namespace MzPeak::Index {

/******************************************************************************/
const static char* INDEX_FILE_NAME = "mzpeak_index.json";

/******************************************************************************/
namespace json = boost::json;

/******************************************************************************/
struct Impl {

  /// Constructor.
  Impl(std::unique_ptr<MzPeak::Archive::Readable> archive)
      : archive_(std::move(archive)) {
    parse_index();
  };

  /// Parse the JSON that makes up the MzPeak index.
  void parse_index();

  // The archive we are reading files out of.
  std::unique_ptr<MzPeak::Archive::Readable> archive_;

  // Parsed file entries.
  std::vector<Schema::File> files_;
};

/******************************************************************************/
Readable::Readable(std::unique_ptr<MzPeak::Archive::Readable> archive)
    : impl_(std::make_unique<Impl>(std::move(archive))) {}

/******************************************************************************/
Readable::~Readable() = default;

/******************************************************************************/
const std::vector<Schema::File>& Readable::files() const { return impl_->files_; }

/******************************************************************************/
void Impl::parse_index() {
  auto file = archive_->read_file(INDEX_FILE_NAME);
  uint8_t buffer[64 * 1024];
  std::optional<std::size_t> bytes;

  json::stream_parser parser;
  boost::system::error_code ec;

  do {
    bytes = file->read(buffer, sizeof(buffer));

    if (bytes.has_value() && *bytes > 0) {
      parser.write(reinterpret_cast<char const*>(buffer), *bytes, ec);
    }

  } while (bytes.has_value() && !ec);

  if (!ec) parser.finish(ec);
  if (ec) throw MzPeak::JsonError(ec.message());

  json::value v = parser.release();
  json::object o = v.as_object();

  if (const auto it = o.find("files"); it != o.end() && it->value().is_array()) {
    const json::array files(it->value().as_array());
    files_.reserve(files.size());

    for (const auto& file : files) {
      if (file.is_object()) {
        files_.push_back(Schema::File(file.as_object()));
      }
    }
  }
}

/******************************************************************************/
std::unique_ptr<Util::Parquet> Readable::parquet(const Schema::File& file) {
  std::unique_ptr<File::Readable> data(impl_->archive_->read_file(file.file_name));
  return std::make_unique<Util::Parquet>(std::move(data), file);
}

} // namespace MzPeak::Index
