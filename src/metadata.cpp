/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/arrow.h"
#include "mzpeak/exception.h"
#include "mzpeak/metadata.h"

#include <parquet/api/reader.h>

namespace MzPeak {

/******************************************************************************/
struct Metadata::Impl {
  Impl(Metadata::readable_archive_t, const MzPeak::Index::File&);
  ~Impl();

  MzPeak::Metadata::readable_archive_t archive_;
  MzPeak::Index::File file_;
  MzPeak::Arrow arrow_;
  std::unique_ptr<MzPeak::Parquet> reader_;

  std::optional<std::size_t> n_entries;
};

/******************************************************************************/
Metadata::Metadata(readable_archive_t archive, const MzPeak::Index::File& file)
    : impl_(std::make_unique<Impl>(std::move(archive), file)) {}

/******************************************************************************/
Metadata::~Metadata() = default;

/******************************************************************************/
Metadata::Impl::Impl(readable_archive_t archive, const MzPeak::Index::File& file)
    : archive_(std::move(archive)), file_(file),
      arrow_(archive_->read_file(file.file_name)) {

  if (file.data_kind != MzPeak::Index::DataKind::Metadata) {
    std::string msg("file is not a metadata file: " + file.file_name);
    throw MzPeak::ParquetError(msg);
  }

  reader_ = arrow_.open();
}

/******************************************************************************/
Metadata::Impl::~Impl() = default;

} // namespace MzPeak
