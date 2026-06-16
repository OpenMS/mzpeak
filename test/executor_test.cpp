/*

This file is part of the package mzpeak.  It is subject to the license
in the LICENSE file found in the top-level directory of this project.

*/

#define BOOST_TEST_MODULE Executor
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/open.h"
#include "mzpeak/query.h"
#include "mzpeak/util/executor.h"
#include "mzpeak/util/parquet.h"
#include "mzpeak/util/planner.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_find_spectrum)
{
  using namespace MzPeak;
  auto index = MzPeak::open("../test/files/small.mzpeak");

  auto entry = std::ranges::find(index.files(), Schema::EntityType::Spectrum,
                                 &Schema::File::entity_type);

  BOOST_TEST((entry != index.files().end()));

  auto parquet = index.parquet(*entry);

  auto index_field = parquet->field("point", "spectrum_index");
  BOOST_TEST(index_field.has_value());

  auto mz_field = parquet->field("point", "mz");
  BOOST_TEST(mz_field.has_value());

  auto query = Query::Builder(*index_field).eq<int64_t>(1);

  Util::Planner planner = parquet->planner(query);
  auto plan = planner.plan();
  BOOST_TEST(plan.ranges.size() == 1ul);

  std::vector<Query::destination_t> projection = {*mz_field};

  Util::Executor executor = parquet->executor(projection);
  const auto& slice = executor.execute(plan);
  BOOST_TEST((slice.fields() == projection));

  auto raw = slice.raw(*mz_field);
  BOOST_TEST((raw != nullptr));
}
