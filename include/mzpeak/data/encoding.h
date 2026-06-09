/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <arrow/array.h>
#include <map>
#include <ranges>
#include <vector>

#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/psi/array_type.h"
#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/util/parquet_types.h"

namespace MzPeak::Data {

/// A vector of Arrow arrays.
using raw_array_type = std::vector<std::shared_ptr<arrow::Array>>;

/// A map of read columns indexed by their column index.
using array_map_type = std::map<int, std::shared_ptr<raw_array_type>>;

template <Schema::PSI::DataType T> class Encoding {
public:
  /// Values that are encoded/decoded by this object.
  using value_type = typename Schema::PSI::data_type_traits<T>::value_type;

  /// Constructor.
  Encoding(const array_map_type& map, const Schema::ArrayIndex& index)
      : map_(map)
      , array_index_(index)
  {
  }

  /// Destructor.
  ~Encoding() = default;

  /**
   * Decode a single array from the given array map.
   */
  std::vector<value_type> decode_array(Schema::PSI::ArrayType) const;

  /**
   * Decode an array using the "point" encoding.
   *
   * You probably want to use `decode_array` instead.
   */
  std::vector<value_type> decode_point(int) const;

  /**
   * Decode an array using the "chunked" encoding.
   *
   * You probably want to use `decode_array` instead.
   */
  // std::vector<value_type>
  // decode_chunked(const std::vector<Schema::ArrayIndex::Array>&) const;

private:
  const array_map_type& map_;
  const Schema::ArrayIndex& array_index_;
};

/******************************************************************************/
template <Schema::PSI::DataType T>
std::vector<typename Encoding<T>::value_type>
Encoding<T>::decode_array(Schema::PSI::ArrayType array_type) const
{
  auto columns = array_index_.columns(array_type);

  if (columns.size() == 1) {
    std::optional<int> index = array_index_.column_index(columns[0]);

    if (index.has_value() &&
        columns[0].buffer_format == Schema::BufferFormat::Point) {
      return decode_point(*index);
    } else {
      std::string msg("unable to decode column, wrong encoding: ");
      throw ParquetError(msg + Schema::PSI::array_type_to_string(array_type));
    }
  } else {
    throw("not implemented");
    // return decode_chunked(arrays);
  }
}

/******************************************************************************/
template <Schema::PSI::DataType T>
std::vector<typename Encoding<T>::value_type>
Encoding<T>::decode_point(int index) const
{
  auto raw = map_.find(index);

  if (raw == map_.end()) {
    std::string msg("index not in column map: " + std::to_string(index) +
                    " should be one of: ");

    std::vector<std::string> keys;
    std::ranges::transform(map_ | std::views::keys, std::back_inserter(keys),
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
    auto ta(Util::parquet_array_cast<T>(array));

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

} // namespace MzPeak::Data
