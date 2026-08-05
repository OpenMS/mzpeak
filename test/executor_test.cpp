/*

This file is part of the package mzpeak.  It is subject to the license
in the LICENSE file found in the top-level directory of this project.

*/

#include "mzpeak/util/projection.h"
#define BOOST_TEST_MODULE Executor
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/open.h"
#include "mzpeak/util/executor.h"
#include "mzpeak/util/manager.h" // IWYU pragma: keep
#include "mzpeak/util/parquet.h"
#include "mzpeak/util/planner.h"
#include "mzpeak/util/query.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_find_spectrum)
{
  using namespace MzPeak;
  auto index = MzPeak::open("../test/files/small.mzpeak");

  auto entry = std::ranges::find(index.files(), Schema::EntityType::Spectrum,
                                 &Schema::File::entity_type);

  BOOST_TEST((entry != index.files().end()));

  auto parquet = index.manager()->parquet(*entry);

  auto index_field = parquet->field("point", "spectrum_index");
  BOOST_TEST(index_field.has_value());

  auto mz_field = parquet->field("point", "mz");
  BOOST_TEST(mz_field.has_value());

  auto query = Util::Query::Builder(*index_field).eq<uint64_t>(1ul);

  Util::Planner planner = parquet->planner(query);
  auto plan = planner.plan();
  BOOST_TEST(plan.ranges.size() == 1ul);

  Util::Projection projection;
  projection.project(*mz_field);

  Util::Executor executor = parquet->executor(projection);
  const auto& slice = executor.execute(plan);
  BOOST_TEST((slice->fields() == projection.get()));

  auto raw = slice->raw(*mz_field);
  BOOST_TEST((raw != nullptr));

  // Decode, dropping null values.
  std::vector<double> mz;
  slice->array<Util::Decoders::Scalar<double>>(*mz_field, mz);
  BOOST_TEST(mz.size() == 15063);
  BOOST_TEST(mz[0] == 200.09, boost::test_tools::tolerance(0.001));
  BOOST_TEST(mz[mz.size() - 1] == 1999.81, boost::test_tools::tolerance(0.001));
}

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_read_uint8_t)
{
  using namespace MzPeak;
  auto index = MzPeak::open("../test/files/small.mzpeak");

  auto entry = std::ranges::find(index.files(), "spectra_metadata.parquet",
                                 &Schema::File::file_name);

  BOOST_TEST((entry != index.files().end()));

  auto parquet = index.manager()->parquet(*entry);

  auto index_field = parquet->field("root", "index");
  BOOST_TEST(index_field.has_value());

  auto ms_level = parquet->field("root", "ms_level");
  BOOST_TEST(ms_level.has_value());

  auto query = Util::Query::Builder(*index_field).eq<uint64_t>(0ul);
  Util::Planner planner = parquet->planner(query);
  auto plan = planner.plan();
  BOOST_TEST(plan.ranges.size() == 1ul);

  Util::Projection projection;
  projection.project(*ms_level);

  Util::Executor executor = parquet->executor(projection);
  const auto& slice = executor.execute(plan);
  BOOST_TEST((slice->fields() == projection.get()));

  auto raw = slice->raw(*ms_level);
  BOOST_TEST((raw != nullptr));

  std::vector<uint8_t> levels;
  slice->array<Util::Decoders::Scalar<uint8_t>>(*ms_level, levels);
  BOOST_TEST(levels.size() == 1);
  BOOST_TEST(levels[0] == 1);
}
