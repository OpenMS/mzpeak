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
  Impl(MzPeak::Archive::Readable& archive) : archive_(archive) { parse_index(); };

  /// Parse the JSON that makes up the MzPeak index.
  void parse_index();

  // The archive we are reading files out of.
  MzPeak::Archive::Readable& archive_;

  // Parsed file entries.
  std::vector<Index::File> files_;
};

/******************************************************************************/
Readable::Readable(MzPeak::Archive::Readable& archive)
    : impl_(std::make_unique<Impl>(archive)) {}

/******************************************************************************/
Readable::~Readable() = default;

/******************************************************************************/
const std::vector<Index::File>& Readable::files() const { return impl_->files_; }

/******************************************************************************/
void Impl::parse_index() {
  auto file = archive_.read_file(INDEX_FILE_NAME);
  MzPeak::Buffer::Basic buffer;
  std::optional<std::size_t> bytes;

  json::stream_parser parser;
  boost::system::error_code ec;

  do {
    bytes = file->read(buffer.data(), buffer.capacity());

    if (bytes.has_value() && *bytes > 0) {
      parser.write(reinterpret_cast<char const*>(buffer.data()), *bytes, ec);
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
        files_.push_back(Index::File(file.as_object()));
      }
    }
  }
}

} // namespace MzPeak::Index
