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

  BOOST_TEST(indices.size() == 1);
  BOOST_TEST((indices[0] == 0));
}

/******************************************************************************/
// Regression: Op::LE evaluated `v >= bound` (greater-equal logic) in BOTH the
// scalar matcher (query.h match(value_type)) and the range matcher
// (match(pair)).  Both paths are exercised here.
BOOST_AUTO_TEST_CASE(less_equal_predicate_matches_correctly)
{
  using namespace MzPeak;
  using DataType = Schema::PSI::DataType;

  auto index = MzPeak::open("../test/files/small.mzpeak");
  auto entry = std::ranges::find(index.files(), Schema::EntityType::Spectrum,
                                 &Schema::File::entity_type);
  auto parquet = index.parquet(*entry);
  auto column = parquet->array_index().columns()[0];

  auto value = [](long long x) {
    return [x](const Schema::ArrayIndex::Column&) -> std::optional<Query::value_t> {
      return Query::value_t{static_cast<Query::p_int64_t>(x)};
    };
  };
  auto range = [](long long lo, long long hi) {
    return [lo, hi](const Schema::ArrayIndex::Column&) -> std::optional<Query::range_t> {
      return Query::range_t{std::pair<Query::p_int64_t, Query::p_int64_t>{lo, hi}};
    };
  };

  Query le = Query::Predicate<DataType::Int64>::less_equal(column, 5);

  // Scalar matcher.
  BOOST_TEST(le.eval(value(3)) == true);  // 3 <= 5
  BOOST_TEST(le.eval(value(5)) == true);  // boundary inclusive
  BOOST_TEST(le.eval(value(9)) == false); // 9 <= 5 is false

  // Range matcher: a [min,max] range can satisfy "<= 5" iff min <= 5.
  BOOST_TEST(le.eval(range(3, 4)) == true);  // min 3 <= 5
  BOOST_TEST(le.eval(range(6, 9)) == false); // min 6 <= 5 is false
}

/******************************************************************************/
// Regression: negating a compound query was ignored because the AND/OR switch
// returned before the not_ handling ran.  Both the AND and OR branches changed,
// so both are exercised here.
BOOST_AUTO_TEST_CASE(negation_applies_to_compound_queries)
{
  using namespace MzPeak;
  using DataType = Schema::PSI::DataType;

  auto index = MzPeak::open("../test/files/small.mzpeak");
  auto entry = std::ranges::find(index.files(), Schema::EntityType::Spectrum,
                                 &Schema::File::entity_type);
  auto parquet = index.parquet(*entry);
  auto column = parquet->array_index().columns()[0];

  auto value = [](long long x) {
    return [x](const Schema::ArrayIndex::Column&) -> std::optional<Query::value_t> {
      return Query::value_t{static_cast<Query::p_int64_t>(x)};
    };
  };

  // AND branch.
  Query both = Query::Predicate<DataType::Int64>::equal_to(column, 3) &&
               Query::Predicate<DataType::Int64>::greater_equal(column, 1);
  BOOST_TEST(both.eval(value(3)) == true);     // 3 == 3 && 3 >= 1
  BOOST_TEST((!both).eval(value(3)) == false); // negation must invert the AND
  BOOST_TEST((!both).eval(value(7)) == true);  // 7 != 3 -> AND false -> negated true

  // OR branch.
  Query either = Query::Predicate<DataType::Int64>::equal_to(column, 3) ||
                 Query::Predicate<DataType::Int64>::equal_to(column, 5);
  BOOST_TEST(either.eval(value(3)) == true);      // 3 == 3
  BOOST_TEST((!either).eval(value(3)) == false);  // negation must invert the OR
  BOOST_TEST((!either).eval(value(7)) == true);   // 7 in neither -> OR false -> negated true
}
