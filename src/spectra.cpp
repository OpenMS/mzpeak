/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <memory>

#include "mzpeak/data/signals.h"
#include "mzpeak/spectra.h"
#include "mzpeak/spectrum.h"
#include "mzpeak/util/enumerable_proxy.h"

namespace MzPeak {

/******************************************************************************/
Spectra::Spectra(std::unique_ptr<Data::Signals> data,
                 std::shared_ptr<Util::Manager> manager)
    : EnumerableProxy(
          0, std::bind(std::mem_fn(&Spectra::fetch), this, std::placeholders::_1))
    , data_(std::move(data))
    , manager_(std::move(manager))
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

  // Sort so that arrays with buffer priority come first.
  std::ranges::sort(dims, [](auto& a, auto& b) {
    return a.array_type < b.array_type && a.buffer_priority > b.buffer_priority;
  });

  // Remove duplicates (on buffer priority).
  const auto to_erase = std::ranges::unique(dims, [](const auto& a, const auto& b) {
    return a.array_type == b.array_type;
  });

  dims.erase(to_erase.begin(), to_erase.end());

  std::unique_ptr<Util::Slice> slice = data_->select(dims, data_->index().eq(index));
  return Spectrum(index, manager_, data_, dims, std::move(slice));
}

} // namespace MzPeak
