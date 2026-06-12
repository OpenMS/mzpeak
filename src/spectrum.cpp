/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <vector>

#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/spectrum.h"
#include "mzpeak/util/encoding.h"

namespace MzPeak {

/******************************************************************************/
inline std::vector<Spectrum::mz_type> decode_mz(const Schema::ArrayIndex& index,
                                                Util::array_map_type& map)
{
  // FIXME: Remove raw mz values.
  Util::Encoding<Schema::PSI::DataType::Float64> enc(map, index);
  return enc.decode_array(Schema::PSI::ArrayType::Mz);
}

/******************************************************************************/
inline std::vector<Spectrum::intensity_type>
decode_intensity(const Schema::ArrayIndex& index, Util::array_map_type& map)
{
  // FIXME: Remove raw intensity values.
  Util::Encoding<Schema::PSI::DataType::Float32> enc(map, index);
  return enc.decode_array(Schema::PSI::ArrayType::Intensity);
}

/******************************************************************************/
Spectrum::Spectrum(const Schema::ArrayIndex& idx,
                   std::unique_ptr<Util::array_map_type> map)
    : array_index_(idx)
    , map_(std::move(map))
    , mz_(decode_mz(array_index_, *map_))
    , intensity_(decode_intensity(array_index_, *map_))
{
}

/******************************************************************************/
const std::vector<Spectrum::mz_type>& Spectrum::mz() const { return mz_; }

/******************************************************************************/
const std::vector<Spectrum::intensity_type>& Spectrum::intensity() const
{
  return intensity_;
}

/******************************************************************************/
const Util::array_map_type& Spectrum::raw_encoded_arrays() const { return *map_; }

/******************************************************************************/
const Schema::ArrayIndex& Spectrum::array_index() const { return array_index_; }

} // namespace MzPeak
