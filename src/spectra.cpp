/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include "mzpeak/spectra.h"
#include "mzpeak/spectrum.h"
#include "mzpeak/util/enumerable_proxy.h"
#include <memory>

namespace MzPeak {

/******************************************************************************/
Spectra::Spectra() {}

/******************************************************************************/
Spectra::Spectra(std::unique_ptr<Util::Parquet> parquet)
    : EnumerableProxy(
          0, std::bind(std::mem_fn(&Spectra::fetch), this, std::placeholders::_1))
    , data_(std::make_shared<Util::DataArrays>(std::move(parquet)))
{

  // Update the record count.
  resize(data_->record_count());
}

/******************************************************************************/
Spectrum Spectra::fetch(std::size_t index)
{
  // FIXME: Write a better way of getting the spectrum index
  auto array_index(data_->array_index());
  auto spectra_index_column = array_index.columns()[0];

  using enum Schema::PSI::DataType;
  Query query = Query::Predicate<Int64>::equal_to(spectra_index_column, index);

  auto map = data_->read_arrays(query, array_index.columns());
  return Spectrum(array_index, std::move(map));
}

} // namespace MzPeak
