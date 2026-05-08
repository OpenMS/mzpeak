/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/index/data_kind.h"

namespace MzPeak::Index {

/******************************************************************************/
std::string data_kind_to_string(DataKind dk) {
  using enum DataKind;

  switch (dk) {
  case DataArray:
    return "data arrays";
  case Peaks:
    return "peaks";
  case Metadata:
    return "metadata";
  case Proprietary:
    return "proprietary";
  case Other:
    return "other";
  }

  // Make the compiler happy:
  return "other";
}

/******************************************************************************/
DataKind data_kind_from_string(const std::string_view& s) {
  using enum DataKind;

  if (s == "data arrays") {
    return DataArray;
  } else if (s == "peaks") {
    return Peaks;
  } else if (s == "metadata") {
    return Metadata;
  } else if (s == "proprietary") {
    return Proprietary;
  } else {
    return Other;
  }
}

} // namespace MzPeak::Index
