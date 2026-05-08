/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/index/entity_type.h"
#define BOOST_TEST_MODULE Parquet
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/arrow.h"
#include "mzpeak/zip.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_open_parque) {
  MzPeak::Archive::Zip zip("../test/files/small.mzpeak");
  MzPeak::Arrow arrow(zip.read_file("spectra_data.parquet"));

  auto file = arrow.open();
  std::size_t count = file->num_entities(MzPeak::Index::EntityType::Spectrum);

  BOOST_TEST(count == 48);

  // std::size_t max(std::ranges::max(
  //     file->map_row_group_metadata<int64_t>(&parquet::RowGroupMetaData::num_rows)));
  //
  // BOOST_TEST(max == count);
}
