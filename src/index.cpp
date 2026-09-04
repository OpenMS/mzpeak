/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <memory>
#include <utility>

#include "mzpeak/data/signals.h"
#include "mzpeak/exception.h"
#include "mzpeak/index.h"
#include "mzpeak/io/archive.h"
#include "mzpeak/spectra.h"
#include "mzpeak/util/manager.h"

namespace MzPeak {

/******************************************************************************/
std::string source_to_file_name(Index::SpectraSource source)
{
  using enum Index::SpectraSource;

  switch (source) {
  case Data:
    return "spectra_data.parquet";
  case Peaks:
    return "spectra_peaks.parquet";
  }

  std::unreachable();
}

/******************************************************************************/
Index::Index(std::unique_ptr<MzPeak::IO::Archive> archive)
    : manager_(std::make_shared<Util::Manager>(std::move(archive)))
{
}

/******************************************************************************/
const std::vector<Schema::File>& Index::files() const { return manager_->files(); }

/******************************************************************************/
std::vector<Schema::File>::const_iterator
Index::find_file(Schema::EntityType::Type et, Schema::DataKind::Type dk) const
{
  return manager_->find_file(et, dk);
}

/******************************************************************************/
bool Index::has_spectra(SpectraSource source) const
{
  auto data_it = manager_->find_file(source_to_file_name(source));
  return data_it != manager_->files().end();
}

/******************************************************************************/
Spectra Index::spectra(SpectraSource source) const
{
  std::string file_name(source_to_file_name(source));
  auto data_it = manager_->find_file(file_name);

  if (data_it == manager_->files().end()) {
    throw InvalidFormatError("missing file spectra file: " + file_name);
  }

  std::unique_ptr<Data::Signals> data =
      std::make_unique<Data::Signals>(manager_->parquet(*data_it));

  return Spectra(std::move(data), manager_);
}

/******************************************************************************/
std::shared_ptr<Util::Manager> Index::manager() const { return manager_; }

} // namespace MzPeak
