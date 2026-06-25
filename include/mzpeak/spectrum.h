/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <memory>

#include "mzpeak/data/signals.h"
#include "mzpeak/metadata/spectrum.h"
#include "mzpeak/metadata/table.h"
#include "mzpeak/util/slice.h"

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
           const Data::Signals&,
           const std::vector<Data::Dimension>&,
           std::unique_ptr<Util::Slice>,
           std::shared_ptr<Metadata::Table>);

private:
  uint64_t index_;
  std::shared_ptr<Metadata::Table> md_table_;
  Metadata::Spectrum md_spec_;
  std::vector<double> mz_;
  std::vector<float> intensity_;
};

} // namespace MzPeak
