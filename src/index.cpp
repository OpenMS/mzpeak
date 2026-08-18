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
#include "mzpeak/spectra.h"
#include "mzpeak/util/manager.h"

namespace MzPeak {

/******************************************************************************/
Index::Index(std::unique_ptr<MzPeak::IO::Archive> archive)
    : manager_(std::make_shared<Util::Manager>(std::move(archive)))
{
}

/******************************************************************************/
const std::vector<Schema::File>& Index::files() const { return manager_->files(); }

/******************************************************************************/
std::vector<Schema::File>::const_iterator Index::find(std::string_view name) const
{
  return manager_->find_file(name);
}

/******************************************************************************/
Spectra Index::spectra() const
{
  auto data_it = manager_->find_file("spectra_data.parquet");

  if (data_it == manager_->files().end()) {
    throw ParquetError("missing files: spectra_data.parquet");
  }

  std::unique_ptr<Data::Signals> data =
      std::make_unique<Data::Signals>(manager_->parquet(*data_it));

  return Spectra(std::move(data), manager_);
}

/******************************************************************************/
std::shared_ptr<Util::Manager> Index::manager() const { return manager_; }

} // namespace MzPeak
