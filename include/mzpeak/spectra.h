/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include "mzpeak/spectrum.h"
#include "mzpeak/util/data_arrays.h"
#include "mzpeak/util/enumerable_proxy.h"

namespace MzPeak {

class Spectrum;

/**
 * Access all spectra in a MzPeak file.
 */
class Spectra final : public Util::EnumerableProxy<Spectrum> {
public:
  /// Default constructor.
  Spectra();

  /// Destructor.
  ~Spectra() = default;

public:
  /// Low-level constructor from a Parquet file.
  explicit Spectra(std::unique_ptr<Util::Parquet> parquet);

private:
  // Internal data access.
  std::shared_ptr<Util::DataArrays> data_;

  // Function to fetch a specific spectrum.
  Spectrum fetch(std::size_t);
};

} // namespace MzPeak
