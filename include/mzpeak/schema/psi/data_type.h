/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <optional>
#include <string>
#include <type_traits>

#include "mzpeak/exception.h"

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

  /// MS:1000521
  ///
  /// 32-bit precision little-endian floating point conforming to
  /// IEEE-754.
  Float32,

  /// MS:1000522
  ///
  /// Signed 64-bit little-endian integer.
  Int64,

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
 * Convert to a DataType from a parquet type enum.
 */
std::optional<DataType> data_type_from_parquet(int);

/**
 * Compile-time information about the DataType type.
 */
template <DataType T> struct data_type_traits;

template <> struct data_type_traits<DataType::Int32> {
  using value_type = int32_t;
};

template <> struct data_type_traits<DataType::Float32> {
  using value_type = float;
};

template <> struct data_type_traits<DataType::Int64> {
  using value_type = int64_t;
};

template <> struct data_type_traits<DataType::Float64> {
  using value_type = double;
};

template <> struct data_type_traits<DataType::ASCII> {
  using value_type = char*;
};

/// Helper for constant dispatching.
template <DataType T> using data_type_constant = std::integral_constant<DataType, T>;

/**
 * Dispatch a function that works with a specific data type.
 */
template <typename Fn, typename... Args>
decltype(auto) dispatch(DataType t, Fn&& func, Args&&... args)
{
  switch (t) {
  case DataType::Int32:
    return std::forward<Fn>(func).template operator()<DataType::Int32>(
        std::forward<Args>(args)...);
  case DataType::Float32:
    return std::forward<Fn>(func).template operator()<DataType::Float32>(
        std::forward<Args>(args)...);
  case DataType::Int64:
    return std::forward<Fn>(func).template operator()<DataType::Int64>(
        std::forward<Args>(args)...);
  case DataType::Float64:
    return std::forward<Fn>(func).template operator()<DataType::Float64>(
        std::forward<Args>(args)...);
  case DataType::ASCII:
    return std::forward<Fn>(func).template operator()<DataType::ASCII>(
        std::forward<Args>(args)...);
  }

  throw TypeError("unknown DataType: " + data_type_to_string(t));
}

} // namespace MzPeak::Schema::PSI
