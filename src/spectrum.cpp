/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <memory>
#include <vector>

#include "mzpeak/data/encoding.h"
#include "mzpeak/metadata/spectrum.h"
#include "mzpeak/spectrum.h"
#include "mzpeak/util/manager.h"

namespace MzPeak {

/******************************************************************************/
const static char* PRIMARY_METADATA_FILE = "spectra_metadata.parquet";

/******************************************************************************/
Spectrum::Spectrum(uint64_t index,
                   std::shared_ptr<Util::Manager> manager,
                   std::shared_ptr<Data::Signals> data,
                   const std::vector<Data::ArrayIndex::Dimension>& dims,
                   std::unique_ptr<Util::Slice> slice)
    : index_(index)
    , manager_(std::move(manager))
    , mz_()
    , intensity_()
    , ms_level_(0)
{
  auto meta_it = manager_->find_file(PRIMARY_METADATA_FILE);

  if (meta_it == manager_->files().end()) {
    throw ParquetError("missing necessary mzpeak file: " +
                       std::string(PRIMARY_METADATA_FILE));
  }

  Metadata::Spectrum meta(manager_->parquet(*meta_it), index_);
  ms_level_ = meta.ms_level().value_or(0);

  Data::Encoding::Decoder<double> decoder(
      std::move(data), std::move(slice),
      Util::DeltaEstimator<double>(meta.delta_model()));

  for (auto& dim : dims) {
    if (dim.array_type == Schema::PSI::ArrayType::Mz) {
      decoder.decimal(dim, mz_);
    } else if (dim.array_type == Schema::PSI::ArrayType::Intensity) {
      decoder.decimal(dim, intensity_);
    } else {
      // FIXME: should we throw an exception here?
      continue;
    }
  }
}

/******************************************************************************/
const std::vector<double>& Spectrum::mz() const { return mz_; }

/******************************************************************************/
const std::vector<float>& Spectrum::intensity() const { return intensity_; }

/******************************************************************************/
uint8_t Spectrum::ms_level() const { return ms_level_; }

} // namespace MzPeak
