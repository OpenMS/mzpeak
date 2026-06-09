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
class Arrays;
class Metadata;
} // namespace MzPeak::Data

namespace MzPeak {

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
  explicit Spectra(std::unique_ptr<Data::Arrays>, std::unique_ptr<Data::Metadata>);

private:
  // Internal data access.
  std::shared_ptr<Data::Arrays> data_;
  std::shared_ptr<Data::Metadata> meta_;

  // Function to fetch a specific spectrum.
  Spectrum fetch(std::size_t);
};

} // namespace MzPeak
