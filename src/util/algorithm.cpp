/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/util/algorithm.h"

namespace MzPeak::Util::Algorithm {

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
