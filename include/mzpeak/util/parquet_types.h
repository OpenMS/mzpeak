/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <parquet/metadata.h>
#include <parquet/schema.h>
#include <parquet/statistics.h>
#include <parquet/types.h>

#include "mzpeak/exception.h"
#include "mzpeak/schema/psi/data_type.h"

namespace MzPeak::Util {

namespace PSI = MzPeak::Schema::PSI;

/// Compile-time conversion between PSI data types and Parquet data
/// types.
template <PSI::DataType T> struct psi_to_parquet_tag;

template <> struct psi_to_parquet_tag<PSI::DataType::Int32> {
  using type = parquet::Int32Type;
};

template <> struct psi_to_parquet_tag<PSI::DataType::Float32> {
  using type = parquet::FloatType;
};

template <> struct psi_to_parquet_tag<PSI::DataType::Int64> {
  using type = parquet::Int64Type;
};

template <> struct psi_to_parquet_tag<PSI::DataType::Float64> {
  using type = parquet::DoubleType;
};

/// Compile-time mapping to get back to the physical type.
template <typename T> struct parquet_to_physical_tag;

template <> struct parquet_to_physical_tag<parquet::Int32Type> {
  static constexpr parquet::Type::type value = parquet::Type::INT32;
};

template <> struct parquet_to_physical_tag<parquet::FloatType> {
  static constexpr parquet::Type::type value = parquet::Type::FLOAT;
};

template <> struct parquet_to_physical_tag<parquet::Int64Type> {
  static constexpr parquet::Type::type value = parquet::Type::INT64;
};

template <> struct parquet_to_physical_tag<parquet::DoubleType> {
  static constexpr parquet::Type::type value = parquet::Type::DOUBLE;
};

/**
 * Validate that the expected PSI type matches the given Parquet type.
 * The `path` parameter is used in the exception thrown when the types
 * don't match.
 */
template <PSI::DataType T>
void runtime_assert_type(parquet::Type::type actual_type, const std::string& path) {
  constexpr parquet::Type::type expected_type =
      parquet_to_physical_tag<typename psi_to_parquet_tag<T>::type>::value;

  if (expected_type != actual_type) {
    std::string msg("while inspecting column: " + path + ": ");
    msg += "expected type doesn't match column type: ";
    msg += parquet::TypeToString(expected_type) + " != ";
    msg += parquet::TypeToString(actual_type);
    throw ParquetError(msg);
  }
}

/// Parquect statistics cast to the correct type.
template <PSI::DataType T>
using parquet_statistics_t =
    parquet::TypedStatistics<typename psi_to_parquet_tag<T>::type>;

/**
 * Cast Parquet statistics to the correctly typed version.
 *
 * If the requested type and actual column type don't match an
 * exception is thrown.
 */
template <PSI::DataType T>
const parquet_statistics_t<T>*
parquet_statistics_cast(const parquet::ColumnChunkMetaData& column,
                        const parquet::Statistics& stats) {
  runtime_assert_type<T>(column.type(), column.path_in_schema()->ToDotString());
  return static_cast<const parquet_statistics_t<T>*>(&stats);
}

} // namespace MzPeak::Util
