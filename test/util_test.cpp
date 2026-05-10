/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE Util
#include <arrow/io/api.h>
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/util/enumerable_proxy.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(enumerable_proxy_simple) {
  std::vector<int> v1{0, 1, 2, 3, 4, 5}, v2;
  v2.reserve(v1.size());

  MzPeak::Util::EnumerableProxy<int, int> ep(v1.size(),
                                             [v1](std::size_t n) { return v1[n]; });

  for (auto i : ep) {
    v2.push_back(i);
  }

  BOOST_TEST(v1 == v2);
}
