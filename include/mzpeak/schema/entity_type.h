/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <string>

namespace MzPeak::Schema {

/**
 * The type of data entity stored in the file.
 */
enum class EntityType {
  /// Mass spectra.
  Spectrum,

  /// Chromatograms or other measurements over time.
  Chromatogram,

  /// Similar to Spectrum except the unit of measure is a wavelength
  /// measurement.
  WavelengthSpectrum,

  /// Unspecified.
  Other
};

/**
 * Convert a EntityType to a string.
 */
std::string entity_type_to_string(EntityType);

/**
 * Parse an EntityType from a string view.
 */
EntityType entity_type_from_string(std::string_view);

} // namespace MzPeak::Schema
