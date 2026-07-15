/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include "mzpeak/spectrum.h"
#include "mzpeak/util/enumerable_proxy.h"

// Forward declarations:
namespace MzPeak::Data {
class Signals;
} // namespace MzPeak::Data

namespace MzPeak::Metadata {
class Table;
}

namespace MzPeak {

/**
 * Access all spectra in a MzPeak file.
 */
class Spectra final : public Util::EnumerableProxy<Spectrum> {
public:
  /// Low-level constructor from a Parquet file.
  explicit Spectra(std::unique_ptr<Data::Signals>, std::unique_ptr<Metadata::Table>);

private:
  // Internal data access.
  std::shared_ptr<Data::Signals> data_;
  std::shared_ptr<Metadata::Table> meta_;

  // Function to fetch a specific spectrum.
  Spectrum fetch(uint64_t);
};

} // namespace MzPeak
