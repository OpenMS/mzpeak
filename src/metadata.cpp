/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/exception.h"
#include "mzpeak/metadata.h"
#include "mzpeak/util/parquet.h"

#include <parquet/api/reader.h>

namespace MzPeak {

/******************************************************************************/
struct Metadata::Impl {
  Impl(Metadata::readable_archive_t, const MzPeak::Schema::File&);
  ~Impl();

  MzPeak::Metadata::readable_archive_t archive_;
  MzPeak::Schema::File file_;
  MzPeak::Util::Arrow arrow_;
  std::unique_ptr<MzPeak::Util::Parquet> reader_;

  std::optional<std::size_t> n_entries;
};

/******************************************************************************/
Metadata::Metadata(readable_archive_t archive, const MzPeak::Schema::File& file)
    : impl_(std::make_unique<Impl>(std::move(archive), file)) {}

/******************************************************************************/
Metadata::~Metadata() = default;

/******************************************************************************/
Metadata::Impl::Impl(readable_archive_t archive, const MzPeak::Schema::File& file)
    : archive_(std::move(archive)), file_(file),
      arrow_(archive_->read_file(file.file_name)) {

  if (file.data_kind != MzPeak::Schema::DataKind::Metadata) {
    std::string msg("file is not a metadata file: " + file.file_name);
    throw MzPeak::ParquetError(msg);
  }

  reader_ = std::make_unique<MzPeak::Util::Parquet>(arrow_);
}

/******************************************************************************/
Metadata::Impl::~Impl() = default;

} // namespace MzPeak
