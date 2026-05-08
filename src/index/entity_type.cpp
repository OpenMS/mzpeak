/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/index/entity_type.h"

namespace MzPeak::Index {

std::string entity_type_to_string(EntityType et) {
  using enum EntityType;

  switch (et) {
  case Spectrum:
    return "spectrum";
  case Chromatogram:
    return "chromatogram";
  case WavelengthSpectrum:
    return "wavelength spectrum";
  case Other:
    return "other";
  }

  // Make the compiler happy.
  return "other";
}

EntityType entity_type_from_string(const std::string_view& s) {
  using enum EntityType;

  if (s == "spectrum") {
    return Spectrum;
  } else if (s == "chromatogram") {
    return Chromatogram;
  } else if (s == "wavelength spectrum") {
    return WavelengthSpectrum;
  } else {
    return Other;
  }
}
} // namespace MzPeak::Index
