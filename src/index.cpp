/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <memory>

#include "mzpeak/data/signals.h"
#include "mzpeak/exception.h"
#include "mzpeak/index.h"
#include "mzpeak/io/archive.h"
#include "mzpeak/metadata/table.h"

/*
 * Boost JSON:
 *   https://www.boost.org/doc/libs/latest/libs/json/doc/html/index.html
 */
namespace MzPeak {

/******************************************************************************/
const static char* INDEX_FILE_NAME = "mzpeak_index.json";

/******************************************************************************/
namespace json = boost::json;

/******************************************************************************/
struct Index::Impl {

  /// Constructor.
  Impl(std::unique_ptr<MzPeak::IO::Archive> archive)
      : archive_(std::move(archive))
      , files_()
  {
    parse_index();
  }

  /// Parse the JSON that makes up the MzPeak index.
  void parse_index();

  // The archive we are reading files out of.
  std::unique_ptr<MzPeak::IO::Archive> archive_;

  // Parsed file entries.
  std::vector<Schema::File> files_;

  // Return an iterator to the requested file.
  std::vector<Schema::File>::const_iterator find_file(const std::string_view& name)
  {
    return std::ranges::find(files_, name, &Schema::File::file_name);
  }
};

/******************************************************************************/
Index::Index(std::unique_ptr<MzPeak::IO::Archive> archive)
    : impl_(std::make_unique<Impl>(std::move(archive)))
{
}

/******************************************************************************/
Index::~Index() = default;

/******************************************************************************/
const std::vector<Schema::File>& Index::files() const { return impl_->files_; }

/******************************************************************************/
void Index::Impl::parse_index()
{
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

    for (const auto& file_obj : files) {
      if (file_obj.is_object()) {
        files_.push_back(Schema::File(file_obj.as_object()));
      }
    }
  }
}

/******************************************************************************/
Spectra Index::spectra() const
{
  auto data_it = impl_->find_file("spectra_data.parquet");
  auto meta_it = impl_->find_file("spectra_metadata.parquet");

  if (data_it == impl_->files_.end()) {
    throw ParquetError("missing files: spectra_data.parquet");
  }

  std::unique_ptr<Data::Signals> data =
      std::make_unique<Data::Signals>(parquet(*data_it));
  std::unique_ptr<Metadata::Table> meta = nullptr;

  if (meta_it != impl_->files_.end()) {
    meta = std::make_unique<Metadata::Table>(parquet(*meta_it));
  }

  return Spectra(std::move(data), std::move(meta));
}

/******************************************************************************/
std::unique_ptr<Util::Parquet> Index::parquet(const Schema::File& file) const
{
  std::unique_ptr<IO::File> data(impl_->archive_->read_file(file.file_name));
  return std::make_unique<Util::Parquet>(std::move(data), file);
}

} // namespace MzPeak
