/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <arrow/array.h>
#include <iterator>
#include <ranges>

#include "mzpeak/query.h"
#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/buffer_format.h"
#include "mzpeak/util/parquet.h"
#include "mzpeak/util/parquet_types.h"

namespace MzPeak::Util {

/**
 * Low-level access to a single data table in a Parquet file.
 */
class DataArrays {
public:
  /// Clarify what we mean by Array.
  using Array = Schema::ArrayIndex::Array;

  /// A vector of Arrow arrays.
  using ArrayVector = std::vector<std::shared_ptr<arrow::Array>>;

  /// A map of read columns indexed by their column index.
  using ArrayMap = std::map<int, std::shared_ptr<ArrayVector>>;

  /// Constructor.
  DataArrays(std::unique_ptr<Util::Parquet> parquet);

  /// Destructor.
  ~DataArrays();

  /**
   * Access the ArrayIndex for this data file.
   */
  const Schema::ArrayIndex& array_index() const;

  /**
   * Extract all of the requested arrays from the current table using
   * the given query to limit the resulting data.
   *
   * NOTE: The query should really only contain predicates that match
   * arrays that have a sort ranking of 0.
   */
  std::unique_ptr<ArrayMap> read_arrays(const Query&, const std::vector<Array>&);

  /**
   * Decode a single array from the given array map.
   */
  template <Schema::PSI::DataType T>
  std::vector<typename Schema::PSI::data_type_traits<T>::value_type>
  decode_array(const ArrayMap&, Schema::PSI::ArrayType) const;

  /**
   * Decode an array using the "point" encoding.
   *
   * You probably want to use `decode_array` instead.
   */
  template <Schema::PSI::DataType T>
  std::vector<typename Schema::PSI::data_type_traits<T>::value_type>
  decode_point(const ArrayMap&, int) const;

  /**
   * Decode an array using the "chunked" encoding.
   *
   * You probably want to use `decode_array` instead.
   */
  template <Schema::PSI::DataType T>
  std::vector<typename Schema::PSI::data_type_traits<T>::value_type>
  decode_chunked(const ArrayMap&, const std::vector<Array>&) const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/******************************************************************************/
template <Schema::PSI::DataType T>
std::vector<typename Schema::PSI::data_type_traits<T>::value_type>
DataArrays::decode_array(const ArrayMap& map, Schema::PSI::ArrayType type) const {
  auto arrays = array_index().arrays(type);

  if (arrays.size() == 1) {
    std::optional<int> index = array_index().column_index(arrays[0]);

    if (index.has_value() &&
        arrays[0].buffer_format == Schema::BufferFormat::Point) {
      return decode_point<T>(map, *index);
    } else {
      std::string msg("unable to decode array, wrong encoding: ");
      throw ParquetError(msg + Schema::PSI::array_type_to_string(type));
    }
  } else {
    return decode_chunked<T>(map, arrays);
  }
}

/******************************************************************************/
template <Schema::PSI::DataType T>
std::vector<typename Schema::PSI::data_type_traits<T>::value_type>
DataArrays::decode_point(const ArrayMap& map, int index) const {
  auto raw = map.find(index);

  if (raw == map.end()) {
    std::string msg("index not in array map: " + std::to_string(index) +
                    " should be one of: ");

    std::vector<std::string> keys;
    std::ranges::transform(map | std::views::keys, std::back_inserter(keys),
                           [](int i) -> std::string { return std::to_string(i); });

    // clang++ on macOS does not support std::views::join_with :-(
    for (auto& key : keys) {
      msg += key + " ";
    }

    throw ParquetError(msg);
  }

  std::size_t size{};

  for (const auto& a : *raw->second) {
    size += a->length();
  }

  std::vector<typename Schema::PSI::data_type_traits<T>::value_type> res;
  res.reserve(size);

  for (auto& array : *raw->second) {
    auto ta(parquet_array_cast<T>(array));

    for (int64_t i : std::views::iota(0, ta->length())) {
      if (ta->IsNull(i)) {
        // FIXME: What should we do here?
        res.push_back(0);
      } else {
        res.push_back(ta->Value(i));
      }
    }
  }

  return res;
}

/******************************************************************************/
template <Schema::PSI::DataType T>
std::vector<typename Schema::PSI::data_type_traits<T>::value_type>
DataArrays::decode_chunked(const ArrayMap&, const std::vector<Array>&) const {
  throw("not implemented");
}

} // namespace MzPeak::Util
