/*

This file is part of the package mzpeak.  It is subject to the license
in the LICENSE file found in the top-level directory of this project.

*/

#define BOOST_TEST_MODULE Query
#include <boost/test/included/unit_test.hpp>

#include <functional>
#include <memory>
#include <ranges>

#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/schema/group.h"
#include "mzpeak/util/compat.h" // IWYU pragma: keep
#include "mzpeak/util/query.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(valid_query_logic)
{
  using namespace MzPeak;

  // This makes me want to make the query class a template class.
  Schema::Group::Field field("fake", 0, 0);
  field.data_type(Schema::PSI::DataType::Int32);

  Schema::Column column =
      std::make_pair(nullptr, std::make_shared<Schema::Group::Field>(field));

  std::vector<int32_t> data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  auto get =
      [data](std::size_t index,
             const Schema::Column&) -> Util::Query::Result<Util::Query::value_t> {
    return Util::Query::Result<Util::Query::value_t>(data[index]);
  };

  auto expect =
      [data](std::move_only_function<bool(int32_t)>&& f) -> std::vector<bool> {
    std::vector<bool> res(data.size(), false);

    for (std::size_t i : std::views::iota(0ul, data.size())) {
      res[i] = f(data[i]);
    }

    return res;
  };

  auto run = [&data, &get](const Util::Query& q) -> std::vector<bool> {
    std::vector<bool> results(data.size(), false);

    for (std::size_t i : std::views::iota(0ul, data.size())) {
      auto r = q.eval(std::bind(get, i, std::placeholders::_1));
      results[i] = r.is(true);
    }

    return results;
  };

  { // EQ
    auto e = expect([](auto n) { return n == 1; });
    auto r = run(Util::Query::Builder(column).eq<int32_t>(1));
    BOOST_TEST(r == e, "eq");
  }

  { // GT
    auto e = expect([](auto n) { return n > 5; });
    auto r = run(Util::Query::Builder(column).gt<int32_t>(5));
    BOOST_TEST(r == e, "gt");
  }

  { // LT
    auto e = expect([](auto n) { return n < 5; });
    auto r = run(Util::Query::Builder(column).lt<int32_t>(5));
    BOOST_TEST(r == e, "lt");
  }

  { // GE
    auto e = expect([](auto n) { return n >= 5; });
    auto r = run(Util::Query::Builder(column).ge<int32_t>(5));
    BOOST_TEST(r == e, "ge");
  }

  { // LE
    auto e = expect([](auto n) { return n <= 5; });
    auto r = run(Util::Query::Builder(column).le<int32_t>(5));
    BOOST_TEST(r == e, "le");
  }

  { // Compound AND
    auto q = Util::Query::Builder(column).le<int32_t>(3) &&
             Util::Query::Builder(column).eq<int32_t>(1);

    auto e = expect([](auto n) { return n <= 3 && n == 1; });
    auto r = run(q);
    BOOST_TEST(r == e, "&&");
  }

  { // Compound OR
    auto q = Util::Query::Builder(column).le<int32_t>(3) ||
             Util::Query::Builder(column).gt<int32_t>(5);

    auto e = expect([](auto n) { return n <= 3 || n > 5; });
    auto r = run(q);
    BOOST_TEST(r == e, "||");
  }

  { // Simple negation.
    auto q = !Util::Query::Builder(column).gt<int32_t>(3);
    auto e = expect([](auto n) { return !(n > 3); });
    auto r = run(q);
    BOOST_TEST(r == e, "!");
  }

  { // Negated compound
    auto q = !(Util::Query::Builder(column).lt<int32_t>(3) ||
               Util::Query::Builder(column).gt<int32_t>(5));

    auto e = expect([](auto n) { return !(n < 3 || n > 5); });
    auto r = run(q);
    BOOST_TEST(r == e, "! ||");
  }
}
