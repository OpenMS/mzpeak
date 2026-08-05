/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <memory>
#include <vector>

#include "mzpeak/data/array_index.h"

namespace MzPeak {

namespace Data {
class Signals;
} // namespace Data

namespace Util {
class Slice;
class Manager;
} // namespace Util

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
  const std::vector<float>& intensity() const;

  /**
   * Stage number achieved in a multi stage mass spectrometry
   * acquisition.
   */
  uint8_t ms_level() const;

protected:
  friend class Spectra;

  /// Internal constructor.
  Spectrum(uint64_t index,
           std::shared_ptr<Util::Manager>,
           std::shared_ptr<Data::Signals>,
           const std::vector<Data::ArrayIndex::Dimension>&,
           std::unique_ptr<Util::Slice>);

private:
  uint64_t index_;
  std::shared_ptr<Util::Manager> manager_;
  std::vector<double> mz_;
  std::vector<float> intensity_;
  uint8_t ms_level_;
};

} // namespace MzPeak
