/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/schema/psi/array_type.h"
#define BOOST_TEST_MODULE DataArrays
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/open.h"
#include "mzpeak/util/data_arrays.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_read_mz_array) {
  using namespace MzPeak;

  auto mzpeak = MzPeak::open("../test/files/small.mzpeak");
  auto entry = std::ranges::find(mzpeak.files(), Schema::EntityType::Spectrum,
                                 &Schema::File::entity_type);

  BOOST_TEST((entry != mzpeak.files().end()));

  auto parquet = mzpeak.parquet(*entry);
  Util::DataArrays data(std::move(parquet));

  auto array_index = data.array_index();
  auto spectra_index_array = array_index.arrays()[0];
  auto mz_array = array_index.arrays()[1];

  Query query;
  auto pred = Query::Predicate<Schema::PSI::DataType::Int64>::equal_to(
      spectra_index_array, 0);
  query.push_back(pred);

  auto map = data.read_arrays(query, {mz_array});
  std::vector<double> mz(data.decode_array<Schema::PSI::DataType::Float64>(
      *map, Schema::PSI::ArrayType::Mz));

  BOOST_TEST(mz.size() == 13589);
  BOOST_TEST(mz[0] == 202.607, boost::test_tools::tolerance(0.001));
  BOOST_TEST(mz[mz.size() - 1] == 1999.840, boost::test_tools::tolerance(0.001));
}
