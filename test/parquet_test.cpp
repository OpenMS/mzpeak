/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE Parquet
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/open.h"
#include "mzpeak/util/parquet.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_get_array_index)
{
  using namespace MzPeak;

  auto mzpeak = MzPeak::open("../test/files/small.mzpeak");

  auto entry = std::ranges::find(mzpeak.files(), Schema::EntityType::Spectrum,
                                 &Schema::File::entity_type);

  BOOST_TEST((entry != mzpeak.files().end()));

  auto parquet = mzpeak.parquet(*entry);
  // std::cerr << parquet->array_index_json() << "\n\n";

  Schema::ArrayIndex index(parquet->array_index());

  BOOST_TEST(index.prefix() == "point");
  BOOST_TEST(index.columns().size() == 3ul);

  BOOST_TEST((index.columns()[1].array_name == "m/z array"));
  BOOST_TEST((index.columns()[1].buffer_format == Schema::BufferFormat::Point));
  BOOST_TEST((index.columns()[1].context == Schema::EntityType::Spectrum));
  BOOST_TEST((index.columns()[1].path == "point.mz"));
  BOOST_TEST((index.columns()[1].data_type == Schema::PSI::DataType::Float64));
  BOOST_TEST((index.columns()[1].array_type == Schema::PSI::ArrayType::Mz));
  BOOST_TEST((index.columns()[1].unit == "MS:1000040"));
  BOOST_TEST((index.columns()[1].buffer_priority));
  BOOST_TEST((index.columns()[1].sorting_rank == std::optional{0}));
  BOOST_TEST((index.columns()[1].data_processing_id == std::nullopt));
  BOOST_TEST((index.columns()[1].transform == std::optional{"MS:1003901"}));

  std::size_t count = index.num_entities().value_or(0);
  BOOST_TEST(count == 48ul);
}
