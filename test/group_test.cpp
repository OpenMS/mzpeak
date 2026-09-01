/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#define BOOST_TEST_MODULE Group
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/open.h"
#include "mzpeak/util/manager.h" // IWYU pragma: keep
#include "mzpeak/util/parquet.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_load_all_groups)
{
  using namespace MzPeak::Util;
  auto mzpeak = MzPeak::open("../test/files/small.mzpeak");

  auto entry = mzpeak.manager()->find_file("spectra_metadata.parquet");
  BOOST_TEST((entry != mzpeak.files().end()));

  auto parquet = mzpeak.manager()->parquet(*entry);

  auto groups = parquet->groups();
  BOOST_TEST((groups->size() == 1));

  auto spectrum_index = parquet->field("root", "index");
  BOOST_TEST(spectrum_index.has_value());
  BOOST_TEST((spectrum_index->second->type().has_value()));
  BOOST_TEST((spectrum_index->second->type().value() == MzPeak::Util::Type::UInt64));
}
