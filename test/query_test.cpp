/*

This file is part of the package mzpeak.  It is subject to the license
in the LICENSE file found in the top-level directory of this project.

*/

#define BOOST_TEST_MODULE Query

#include <boost/test/included/unit_test.hpp>

#include "mzpeak/open.h"
#include "mzpeak/query.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_find_spectrum) {
  using namespace MzPeak;
  using DataType = Schema::PSI::DataType;

  auto index = MzPeak::open("../test/files/small.mzpeak");

  auto entry = std::ranges::find(index.files(), Schema::EntityType::Spectrum,
                                 &Schema::File::entity_type);

  BOOST_TEST((entry != index.files().end()));

  auto parquet = index.parquet(*entry);
  Query query(parquet->file_metadata());

  Query::Location loc = query.find([&](Query::Cursor& cursor) {
    auto span = cursor.column_span<DataType::Int64>("point.spectrum_index");
    BOOST_TEST((span.has_value()));

    // NOTE: This file only has one row group.
    if (span->first == 0) {
      return cursor.match();
    } else {
      return cursor.stop();
    }
  });

  BOOST_TEST(loc.row_group_indices.size() == 1);
  BOOST_TEST((loc.row_group_indices[0] == 0));
}
