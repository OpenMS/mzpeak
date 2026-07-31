/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <arrow/array.h>
#include <cstdint>
#include <memory>
#include <vector>

namespace MzPeak::Util::Numpress {

/**
 * Decode a vector of bytes into a vector of doubles.
 *
 * The bytes need to be encoded using the MS-Numpress Linear encoding.
 */
void decode_linear(const std::vector<uint8_t>&, std::vector<double>&);

/**
 * Decode an arrow array of `uint8_t` values.
 */
std::shared_ptr<std::vector<double>>
decode_linear(const std::shared_ptr<arrow::Array>&);

/**
 * Decode and perform type conversion if necessary.
 */
template <typename T>
std::shared_ptr<std::vector<T>>
decode_linear_convert(const std::shared_ptr<arrow::Array>& src)
{
  std::shared_ptr<std::vector<double>> doubles = decode_linear(src);

  if constexpr (std::is_same_v<T, double>) {
    return doubles;
  } else {
    std::shared_ptr<std::vector<T>> result = std::make_shared<std::vector<T>>();
    result->reserve(doubles->size());

    for (const auto& d : *doubles) {
      result->push_back(static_cast<T>(d));
    }

    return result;
  }
};

} // namespace MzPeak::Util::Numpress
