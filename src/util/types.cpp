/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <parquet/schema.h>
#include <parquet/types.h>

#include "mzpeak/util/types.h"

namespace MzPeak::Util {

/******************************************************************************/
std::optional<Type> type_from_parquet(const parquet::schema::PrimitiveNode& node)
{
  using enum Type;

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

} // namespace MzPeak::Util
