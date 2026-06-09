/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <vector>

#include "mzpeak/data/encoding.h"
#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/spectrum.h"

namespace MzPeak {

/******************************************************************************/
inline std::vector<Spectrum::mz_type> decode_mz(const Schema::ArrayIndex& index,
                                                Data::array_map_type& map)
{
  // FIXME: Remove raw mz values.
  Data::Encoding<Schema::PSI::DataType::Float64> enc(map, index);
  return enc.decode_array(Schema::PSI::ArrayType::Mz);
}

/******************************************************************************/
inline std::vector<Spectrum::intensity_type>
decode_intensity(const Schema::ArrayIndex& index, Data::array_map_type& map)
{
  // FIXME: Remove raw intensity values.
  Data::Encoding<Schema::PSI::DataType::Int32> enc(map, index);
  return enc.decode_array(Schema::PSI::ArrayType::Intensity);
}

/******************************************************************************/
Spectrum::Spectrum(const Schema::ArrayIndex& idx,
                   std::unique_ptr<Data::array_map_type> map)
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
const Data::array_map_type& Spectrum::raw_encoded_arrays() const { return *map_; }

/******************************************************************************/
const Schema::ArrayIndex& Spectrum::array_index() const { return array_index_; }

} // namespace MzPeak
