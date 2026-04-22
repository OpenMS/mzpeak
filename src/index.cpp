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
  Impl(MzPeak::Archive::Readable& archive)
      : archive_(archive), json_(parse_index()) {};

  /// Parse the JSON that makes up the MzPeak index.
  json::value parse_index();

  // The archive we are reading files out of.
  MzPeak::Archive::Readable& archive_;

  // The JSON from the index.
  json::value json_;
};

/******************************************************************************/
Readable::Readable(MzPeak::Archive::Readable& archive)
    : impl_(std::make_unique<Impl>(archive)) {}

/******************************************************************************/
Readable::~Readable() = default;

/******************************************************************************/
const json::object Readable::metadata() const {
  const json::object& obj = impl_->json_.as_object();
  return obj.at("metadata").as_object();
}

/******************************************************************************/
json::value Impl::parse_index() {
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
  if (ec) throw MzPeak::Exception::JsonError(ec.message());

  return parser.release();
}

} // namespace MzPeak::Index
