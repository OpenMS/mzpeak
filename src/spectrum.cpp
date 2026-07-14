/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <memory>
#include <vector>

#include "mzpeak/spectrum.h"

namespace MzPeak {

/******************************************************************************/
Spectrum::Spectrum(uint64_t index,
                   std::shared_ptr<Data::Signals> data,
                   const std::vector<Data::ArrayIndex::Dimension>& dims,
                   std::unique_ptr<Util::Slice> slice,
                   std::shared_ptr<Metadata::Table> metadata)
    : index_(index)
    , md_table_(std::move(metadata))
    , md_spec_(md_table_, index_)
    , decoder_(data,
               std::move(slice),
               Util::DeltaEstimator<double>(md_spec_.delta_model()))
{
  for (auto& dim : dims) {
    switch (dim.array_type) {
    case Schema::PSI::ArrayType::Mz:
      decoder_.decimal(dim, mz_);
      break;
    case Schema::PSI::ArrayType::Intensity:
      decoder_.decimal(dim, intensity_);
      break;
    default:
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
uint8_t Spectrum::ms_level() const { return md_spec_.ms_level().value_or(0u); }

} // namespace MzPeak
