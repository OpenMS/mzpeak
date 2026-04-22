/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE Index
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/index.h"
#include "mzpeak/zip.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_parse_json) {
  MzPeak::Archive::Zip zip("../test/files/small.mzpeak");
  MzPeak::Index::Readable index(zip);
  const auto& metadata = index.metadata();

  // FIXME: The small file has no metadata :(
  BOOST_TEST(metadata.empty(), "metadata should be empty but is: " << metadata);
}
