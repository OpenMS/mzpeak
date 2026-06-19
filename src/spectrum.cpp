/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <memory>
#include <vector>

#include "mzpeak/data/encoding.h"
#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/spectrum.h"

namespace MzPeak {

/******************************************************************************/
struct Decode {
  Decode(const Data::Arrays& data,

         std::shared_ptr<Data::Slice> slice)
      : array_index_(data.array_index())
      , struct_map_(data.structs())
      , slice_(slice)
  {
  }

  template <Schema::PSI::DataType T>
  void decode(const Data::Dimension& dim,
              std::vector<typename Data::Encoding<T>::value_type>& v)
  {
    Data::Encoding<T> enc(array_index_, struct_map_, slice_);
    enc.decode_dimension(dim, v);
  }

  std::shared_ptr<Schema::ArrayIndex> array_index_;
  std::shared_ptr<Util::StructMap> struct_map_;
  std::shared_ptr<Data::Slice> slice_;
};

/******************************************************************************/
Spectrum::Spectrum(const Data::Arrays& data,
                   const std::vector<Data::Dimension>& dims,
                   std::unique_ptr<Data::Slice> slice_up)
{
  std::shared_ptr<Data::Slice> slice(std::move(slice_up));

  for (auto& dim : dims) {
    Decode decode(data, slice);

    switch (dim.array_type) {
    case Schema::PSI::ArrayType::Mz:
      // FIXME: What if the values are float32?
      decode.decode<Schema::PSI::DataType::Float64>(dim, mz_);
      break;
    case Schema::PSI::ArrayType::Intensity:
      decode.decode<Schema::PSI::DataType::Int32>(dim, intensity_);
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
const std::vector<int32_t>& Spectrum::intensity() const { return intensity_; }

} // namespace MzPeak
