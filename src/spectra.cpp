/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <memory>

#include "mzpeak/data/signals.h"
#include "mzpeak/metadata/table.h"
#include "mzpeak/spectra.h"
#include "mzpeak/spectrum.h"
#include "mzpeak/util/enumerable_proxy.h"

namespace MzPeak {

/******************************************************************************/
Spectra::Spectra() {}

/******************************************************************************/
Spectra::Spectra(std::unique_ptr<Data::Signals> data,
                 std::unique_ptr<Metadata::Table> meta)
    : EnumerableProxy(
          0, std::bind(std::mem_fn(&Spectra::fetch), this, std::placeholders::_1))
    , data_(std::move(data))
    , meta_(std::move(meta))
{
  // Update the record count.
  resize(data_->record_count());
}

/******************************************************************************/
Spectrum Spectra::fetch(uint64_t index)
{
  // These are the dimensions we'll project by default.
  std::vector<Data::ArrayIndex::Dimension> dims =
      data_->array_index()->dimensions() | std::views::filter([](auto& d) {
        return d.array_type == Schema::PSI::ArrayType::Mz ||
               d.array_type == Schema::PSI::ArrayType::Intensity;
      }) |
      std::ranges::to<std::vector<Data::ArrayIndex::Dimension>>();

  std::unique_ptr<Util::Slice> slice = data_->select(dims, data_->index().eq(index));
  return Spectrum(index, *data_, dims, std::move(slice), meta_);
}

} // namespace MzPeak
