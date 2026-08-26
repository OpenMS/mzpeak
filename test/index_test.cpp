/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE Index
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/index.h"
#include "mzpeak/open.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_parse_json)
{
  auto index = MzPeak::open("../test/files/small.mzpeak");
  const auto& files = index.files();
  BOOST_TEST(!files.empty(), "files should not be empty");
}
