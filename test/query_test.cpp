/*

This file is part of the package mzpeak.  It is subject to the license
in the LICENSE file found in the top-level directory of this project.

*/

#define BOOST_TEST_MODULE Query
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/open.h"
#include "mzpeak/query.h"
#include "mzpeak/util/parquet.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_find_spectrum)
{
  // FIXME: Use proper field access.

  using namespace MzPeak;
  auto index = MzPeak::open("../test/files/small.mzpeak");

  auto entry = std::ranges::find(index.files(), Schema::EntityType::Spectrum,
                                 &Schema::File::entity_type);

  BOOST_TEST((entry != index.files().end()));

  auto parquet = index.parquet(*entry);
  auto index_field = parquet->field("point", "spectrum_index");

  BOOST_TEST(index_field.has_value());

  auto query = Query::Builder(*index_field).eq<int64_t>(1);
  auto indices = parquet->find_row_groups(query);

  BOOST_TEST(indices.size() == 1ul);
  BOOST_TEST((indices[0] == 0));
}
