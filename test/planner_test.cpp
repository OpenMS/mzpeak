/*

This file is part of the package mzpeak.  It is subject to the license
in the LICENSE file found in the top-level directory of this project.

*/

#define BOOST_TEST_MODULE Planner
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/open.h"
#include "mzpeak/util/manager.h" // IWYU pragma: keep
#include "mzpeak/util/parquet.h"
#include "mzpeak/util/planner.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_locate_correct_rows)
{
  using namespace MzPeak;
  auto index = MzPeak::open("../test/files/small.mzpeak");

  auto entry = std::ranges::find(index.files(), "spectra_data.parquet",
                                 &Schema::File::file_name);

  BOOST_TEST((entry != index.files().end()));

  auto parquet = index.manager()->parquet(*entry);

  auto index_field = parquet->field("point", "spectrum_index");
  BOOST_TEST(index_field.has_value());

  auto mz_field = parquet->field("point", "mz");
  BOOST_TEST(mz_field.has_value());

  Util::Query queries[] = {
      Util::Query::Builder(*index_field).eq<uint64_t>(20),
      Util::Query::Builder(*mz_field).gt<double>(0),
  };

  for (auto& query : queries) {
    Util::Planner planner = parquet->planner(query);
    auto plan = planner.plan();

    // Ug, this file is too small to exercise the planner.
    BOOST_TEST(plan.ranges.size() == 1ul);

    auto first = plan.ranges[0];
    BOOST_TEST(first.row_group == 0);
    BOOST_TEST(first.offset == 0);
    BOOST_TEST(first.length == 217710);
  }
}

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_use_two_columns)
{
  using namespace MzPeak;
  auto index = MzPeak::open("../test/files/small.mzpeak");

  auto entry = std::ranges::find(index.files(), "spectra_metadata_scans.parquet",
                                 &Schema::File::file_name);

  BOOST_TEST((entry != index.files().end()));

  auto parquet = index.manager()->parquet(*entry);

  auto index_field = parquet->field("root", "source_index");
  BOOST_TEST(index_field.has_value());

  auto start_time_field = parquet->field("root", "scan_start_time");
  BOOST_TEST(start_time_field.has_value());

  auto query = Util::Query::Builder(*index_field)
                   .eq<uint64_t>(3)
                   .and_then(Util::Query::Builder(*start_time_field).gt<float>(0.0));

  Util::Planner planner = parquet->planner(query);

  auto plan = planner.plan();
  BOOST_TEST(plan.ranges.size() == 1ul);

  // Ug, this file is too small to exercise the planner.
  auto first = plan.ranges[0];
  BOOST_TEST(first.row_group == 0);
  BOOST_TEST(first.offset == 0);
  BOOST_TEST(first.length == 48);
}
