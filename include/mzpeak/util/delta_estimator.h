/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <cmath>
#include <ranges>
#include <vector>

namespace MzPeak::Util {

/**
 * A type-safe wrapper for the NULL decoding regression model.
 */
template <typename T> class DeltaEstimator final {
public:
  /// Constructor.
  explicit DeltaEstimator(const std::vector<T> model)
      : model_(model)
  {
  }

  /// Destructor.
  ~DeltaEstimator() = default;

  /// Predict a delta given a real value.
  T predict(T val) const;

private:
  std::vector<T> model_;
};

/******************************************************************************/
template <typename T> T DeltaEstimator<T>::predict(T base) const
{
  T acc{};

  for (std::size_t i : std::views::iota(0ul, model_.size())) {
    if (i == 0ul) {
      acc += model_[i];
    } else {
      acc += model_[i] * std::pow<T, int32_t>(base, i);
    }
  }

  return acc;
}

} // namespace MzPeak::Util
