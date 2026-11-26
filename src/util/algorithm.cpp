/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/util/algorithm.h"

namespace MzPeak::Util::Algorithm {

/******************************************************************************/
std::generator<const span_t&> spans(const std::vector<bool>& bins)
{
  auto i = std::ranges::find(bins.begin(), bins.end(), true);
  auto j = bins.end();
  auto val = std::make_pair<std::size_t, std::size_t>(0, 0);

  for (; i != bins.end(); i = std::ranges::find(j, bins.end(), true)) {
    j = std::ranges::find(i + 1, bins.end(), false);
    val.first = std::distance(bins.begin(), i);
    val.second = std::distance(bins.begin(), j - 1);
    co_yield val;
  }
}

/******************************************************************************/
std::pair<std::size_t, std::size_t> range_to_request(std::size_t cache_size,
                                                     std::size_t record_size,
                                                     std::size_t index_wanted)
{
  // How many records are in each batch?
  const std::size_t batch_size = cache_size / record_size;

  // Which batch contains the requested index?
  const std::size_t batch = std::ceil((static_cast<double>(index_wanted) + 1.0) /
                                      static_cast<double>(batch_size));

  const std::size_t begin = (batch - 1) * batch_size;
  return std::make_pair(begin, begin + batch_size);
}

} // namespace MzPeak::Util::Algorithm
