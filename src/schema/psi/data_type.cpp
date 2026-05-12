/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/schema/psi/data_type.h"

namespace MzPeak::Schema::PSI {

/******************************************************************************/
std::string data_type_to_string(DataType v) {
  using enum DataType;

  switch (v) {
  case Int32:
    return "MS:1000519";
  case Float32:
    return "MS:1000521";
  case Int64:
    return "MS:1000522";
  case Float64:
    return "MS:1000523";
  case ASCII:
    return "MS:1001479";
  default:
    return "MS:1001479";
  }
}

/******************************************************************************/
DataType data_type_from_string(const std::string_view& s) {
  using enum DataType;

  if (s == "MS:1000519") {
    return Int32;
  } else if (s == "MS:1000521") {
    return Float32;
  } else if (s == "MS:1000522") {
    return Int64;
  } else if (s == "MS:1000523") {
    return Float64;
  } else if (s == "MS:1001479") {
    return ASCII;
  } else {
    return ASCII;
  }
}

} // namespace MzPeak::Schema::PSI
