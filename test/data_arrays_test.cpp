/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE DataArrays
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/data/arrays.h"
#include "mzpeak/open.h"
#include "mzpeak/schema/psi/array_type.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_read_mz_array)
{
  // FIXME: Use proper field access.

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

  auto array_index = data.array_index();

  auto query = Query::Builder(*index_field).eq<int64_t>(0);
  auto map = data.read_arrays(query, {*mz_column->second});

  Data::Encoding<Schema::PSI::DataType::Float64> enc(*map, array_index);
  std::vector<double> mz(enc.decode_array(Schema::PSI::ArrayType::Mz));

  BOOST_TEST(mz.size() == 13589ul);
  BOOST_TEST(mz[0] == 202.607, boost::test_tools::tolerance(0.001));
  BOOST_TEST(mz[mz.size() - 1] == 1999.840, boost::test_tools::tolerance(0.001));
}
