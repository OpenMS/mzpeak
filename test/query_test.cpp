/*

This file is part of the package mzpeak.  It is subject to the license
in the LICENSE file found in the top-level directory of this project.

*/

#define BOOST_TEST_MODULE Query
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/query.h"
#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/util/types.h"
#include <memory>
#include <ranges>

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_find_spectrum)
{
  using namespace MzPeak;

  // This makes me want to make the query class a template class.
  Util::Struct::Field field("fake", 0, 0);
  field.data_type(Schema::PSI::DataType::Int32);

  Util::Column column =
      std::make_pair(nullptr, std::make_shared<Util::Struct::Field>(field));

  std::vector<int32_t> data = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  auto get = [data](std::size_t index,
                    const Util::Column&) -> Query::Result<Query::value_t> {
    return Query::Result<Query::value_t>(data[index]);
  };

  auto run = [&data, &get](const Query& q) -> std::vector<bool> {
    std::vector<bool> results(data.size(), false);

    for (std::size_t i : std::views::iota(0ul, data.size())) {
      auto r = q.eval(std::bind(get, i, std::placeholders::_1));
      results[i] = r.is(true);
    }

    return results;
  };

  {
    auto results = run(Query::Builder(column).eq<int32_t>(1));
    BOOST_TEST(std::ranges::count(results, true) == 1);
    BOOST_TEST(results[1] == true);
  }
}
