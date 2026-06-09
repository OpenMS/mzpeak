/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/data/metadata.h"
#include "mzpeak/exception.h"
#include "mzpeak/util/parquet.h"

#include <parquet/api/reader.h>

namespace MzPeak::Data {

/******************************************************************************/
struct Metadata::Impl {
  Impl(std::unique_ptr<Util::Parquet> parquet);
  ~Impl();

  std::unique_ptr<Util::Parquet> reader_;
  std::optional<std::size_t> n_entries;
};

/******************************************************************************/
Metadata::Metadata(std::unique_ptr<Util::Parquet> parquet)
    : impl_(std::make_unique<Impl>(std::move(parquet)))
{
}

/******************************************************************************/
Metadata::~Metadata() = default;

/******************************************************************************/
Metadata::Impl::Impl(std::unique_ptr<Util::Parquet> parquet)
    : reader_(std::move(parquet))
{
  auto file = reader_->index_file();

  if (file.data_kind != Schema::DataKind::Metadata) {
    std::string msg("file is not a metadata file: " + file.file_name);
    throw ParquetError(msg);
  }
}

/******************************************************************************/
Metadata::Impl::~Impl() = default;

} // namespace MzPeak::Data
