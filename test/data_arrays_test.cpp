/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE DataArrays
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/data/arrays.h"
#include "mzpeak/open.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_read_mz_array)
{
  using namespace MzPeak;

  auto mzpeak = MzPeak::open("../test/files/small.mzpeak");
  auto entry = std::ranges::find(mzpeak.files(), Schema::EntityType::Spectrum,
                                 &Schema::File::entity_type);

  BOOST_TEST((entry != mzpeak.files().end()));

  auto parquet = mzpeak.parquet(*entry);

  Data::Arrays data(std::move(parquet));

  auto index_field = data.field("spectrum_index");
  auto mz_column = data.field("mz");

  BOOST_TEST(index_field.has_value());
  BOOST_TEST(mz_column.has_value());
}
