/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <string>

namespace MzPeak::Util {
enum class Type : int;
}

namespace MzPeak::Schema::PSI {

/**
 * Children of the MS:1000518 type:
 *
 * Encoding type of binary data specifying the binary representation
 * and precision, e.g. 64-bit float.
 */
enum class DataType {
  /// MS:1000519
  ///
  /// Signed 32-bit little-endian integer.
  Int32,

  /// MS:1000522
  ///
  /// Signed 64-bit little-endian integer.
  Int64,

  /// MS:1000521
  ///
  /// 32-bit precision little-endian floating point conforming to
  /// IEEE-754.
  Float32,

  /// MS:1000523
  ///
  /// 64-bit precision little-endian floating point conforming to
  /// IEEE-754.
  Float64,

  /// MS:1001479
  ///
  /// Sequence of zero or more non-zero ASCII characters terminated by
  /// a single null (0) byte.
  ASCII,
};

/**
 * Convert a DataType to a string.
 */
std::string data_type_to_string(DataType);

/**
 * Parse a DataType from a string.
 */
DataType data_type_from_string(const std::string_view&);

/**
 * Convert a DataType to a Util::Type.
 */
Util::Type data_type_to_type(DataType);

} // namespace MzPeak::Schema::PSI
