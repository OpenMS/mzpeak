/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE Util
#include <boost/test/included/unit_test.hpp>

#include <arrow/io/api.h>

#include "mzpeak/util/enumerable_proxy.h"

namespace {
struct EPTest : public MzPeak::Util::EnumerableProxy<int, int> {
  EPTest()
      : EnumerableProxy(0)
  {
    resize(v.size());
  }

  std::vector<int> v{0, 1, 2, 3, 4, 5};
  int fetch(uint64_t i) { return v[i]; }
};
} // namespace

/******************************************************************************/
BOOST_AUTO_TEST_CASE(enumerable_proxy_simple)
{
  EPTest ep_test;
  std::vector<int> v;
  v.reserve(ep_test.v.size());

  for (auto i : ep_test) {
    v.push_back(i);
  }

  BOOST_TEST(ep_test.v.size() == v.size());
  BOOST_TEST(ep_test.v == v);
}
