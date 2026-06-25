/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <parquet/schema.h>
#include <parquet/types.h>

#include "mzpeak/schema/psi/data_type.h"

namespace MzPeak::Schema::PSI {

/******************************************************************************/
std::string data_type_to_string(DataType v)
{
  using enum DataType;

  // FIXME: Custom types

  switch (v) {
  case Int8:
    return "MS:XXXINT8";
  case UInt8:
    return "MS:XXUINT8";
  case Int32:
    return "MS:1000519";
  case UInt32:
    return "MS:XUINT32";
  case Int64:
    return "MS:1000522";
  case UInt64:
    return "MS:XUINT64";
  case Float32:
    return "MS:1000521";
  case Float64:
    return "MS:1000523";
  case ASCII:
    return "MS:1001479";
  default:
    return "MS:1001479";
  }
}

/******************************************************************************/
DataType data_type_from_string(const std::string_view& s)
{
  using enum DataType;

  // FIXME: Custom types

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

/******************************************************************************/
std::optional<DataType>
data_type_from_parquet(const parquet::schema::PrimitiveNode& node)
{
  using enum DataType;

  // First try to use the converted type which is more accurate.
  switch (node.converted_type()) {
  case parquet::ConvertedType::UINT_8:
    return UInt8;
  case parquet::ConvertedType::UINT_32:
    return UInt32;
  case parquet::ConvertedType::UINT_64:
    return UInt64;
  case parquet::ConvertedType::INT_8:
    return Int8;
  case parquet::ConvertedType::INT_32:
    return Int32;
  case parquet::ConvertedType::INT_64:
    return Int64;
  default:
    break;
  }

  // Fall back to the physical type.
  switch (node.physical_type()) {
  case parquet::Type::FLOAT:
    return Float32;
  case parquet::Type::DOUBLE:
    return Float64;
  default:
    break;
  }

  // Fail.
  return {};
}

} // namespace MzPeak::Schema::PSI
