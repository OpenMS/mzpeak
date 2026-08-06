/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/util/manager.h"

namespace MzPeak::Util {

/******************************************************************************/
const static char* INDEX_FILE_NAME = "mzpeak_index.json";

/******************************************************************************/
namespace json = boost::json;

/******************************************************************************/
void parse_index(std::shared_ptr<MzPeak::IO::Archive>& archive,
                 std::vector<Schema::File>& files)
{
  auto file = archive->read_file(INDEX_FILE_NAME);
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
    const json::array file_list(it->value().as_array());
    files.reserve(file_list.size());

    for (const auto& file_obj : file_list) {
      if (file_obj.is_object()) {
        files.push_back(Schema::File(file_obj.as_object()));
      }
    }
  }
}

/******************************************************************************/
Manager::Manager(std::unique_ptr<MzPeak::IO::Archive> archive)
    : archive_(std::move(archive))
    , files_()
{
  parse_index(archive_, files_);
}

/******************************************************************************/
const std::vector<Schema::File>& Manager::files() const { return files_; }

/******************************************************************************/
std::vector<Schema::File>::const_iterator
Manager::find_file(const std::string_view& name) const
{
  return std::ranges::find(files_, name, &Schema::File::file_name);
}

/******************************************************************************/
std::unique_ptr<Util::Parquet> Manager::parquet(const Schema::File& file) const
{
  std::unique_ptr<IO::File> data(archive_->read_file(file.file_name()));
  return std::make_unique<Util::Parquet>(std::move(data), file);
}

} // namespace MzPeak::Util
