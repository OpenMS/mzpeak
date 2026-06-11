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
  using namespace MzPeak;
  using DataType = Schema::PSI::DataType;

  auto index = MzPeak::open("../test/files/small.mzpeak");

  auto entry = std::ranges::find(index.files(), Schema::EntityType::Spectrum,
                                 &Schema::File::entity_type);

  BOOST_TEST((entry != index.files().end()));

  auto parquet = index.parquet(*entry);
  auto array_index = parquet->array_index();
  auto spectra_index_column = array_index.columns()[0];

  Query query = Query::Predicate<DataType::Int64>::equal_to(spectra_index_column, 1);
  auto indices = parquet->find_row_groups(query);

  BOOST_TEST(indices.size() == 1ul);
  BOOST_TEST((indices[0] == 0));
}
