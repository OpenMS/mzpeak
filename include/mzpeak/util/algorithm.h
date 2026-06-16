/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <functional>
#include <iterator>
#include <vector>

namespace MzPeak::Util::Algorithm {

/**
 * Compute all runs of true values in the given vector.  For each run,
 * invoke the given function with index to the first and last true
 * value in the current span.
 */
template <typename Fn, typename... Args>
void spans(const std::vector<bool>& bins, Fn&& func, Args&&... args)
{
  auto i = std::ranges::find(bins.begin(), bins.end(), true);
  auto j = bins.end();

  for (; i != bins.end(); i = std::ranges::find(j, bins.end(), true)) {
    j = std::ranges::find(i + 1, bins.end(), false);
    auto first = std::distance(bins.begin(), i);
    auto last = std::distance(bins.begin(), j - 1);
    std::invoke(func, static_cast<std::size_t>(first), static_cast<size_t>(last),
                args...);
  }
}

} // namespace MzPeak::Util::Algorithm
