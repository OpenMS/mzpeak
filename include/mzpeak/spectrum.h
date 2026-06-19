/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <memory>

#include "mzpeak/data/arrays.h"
#include "mzpeak/data/slice.h"
#include "mzpeak/schema/psi/data_type.h"

namespace MzPeak {

// Forward declaration.
class Spectra;

/**
 * Access to a single spectrum in an MzPeak file.
 */
class Spectrum final {
public:
  /// Destructor.
  ~Spectrum() = default;

  /**
   * Mass-to-charge values.
   */
  const std::vector<double>& mz() const;

  /**
   * Intensity values.
   */
  const std::vector<int32_t>& intensity() const;

  // FIXME: level?

protected:
  friend class Spectra;

  /// Internal constructor.
  Spectrum(const Data::Arrays&,
           const std::vector<Data::Dimension>&,
           std::unique_ptr<Data::Slice>);

private:
  std::vector<double> mz_;
  std::vector<int32_t> intensity_;
};

} // namespace MzPeak
