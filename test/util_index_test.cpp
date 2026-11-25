/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE UtilIndex
#include <boost/test/included/unit_test.hpp>

#include <boost/json.hpp>

#include "mzpeak/util/index.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_merge_nodes)
{
  using namespace MzPeak::Util;

  Index idx;
  const Index::key_type key = 0;

  auto insert = [&](int min, int max, Index::row_group_id rg) -> void {
    idx.insert(key, min, max, rg);
  };

  auto assert_single_node = [&](int min, int max,
                                const Index::matching_groups& groups) -> void {
    auto loc = idx.get().find(key);
    BOOST_TEST_REQUIRE((loc != idx.get().end()));

    auto vec = loc->second;
    BOOST_TEST_REQUIRE(vec->size() == 1, "for min/max " << min << "/" << max
                                                        << " size() != 1 "
                                                        << idx.to_json());

    auto node = vec->front();
    any_value_type any_min(min);
    any_value_type any_max(max);
    BOOST_TEST((any_min == node.min_value_));
    BOOST_TEST((any_max == node.max_value_));
    BOOST_TEST(groups == node.row_groups_);
  };

  // All inserts should cause merges (after the first insert of course).
  insert(3, 3, 0);
  insert(3, 4, 1);
  assert_single_node(3, 4, {0, 1});

  insert(2, 4, 2);
  assert_single_node(2, 4, {0, 1, 2});

  insert(4, 6, 3);
  assert_single_node(2, 6, {0, 1, 2, 3});

  insert(3, 5, 3);
  assert_single_node(2, 6, {0, 1, 2, 3});

  // This should not get merged.
  insert(7, 12, 4);
  auto loc = idx.get().find(key);
  BOOST_TEST_REQUIRE((loc != idx.get().end()));
  BOOST_TEST(loc->second->size() == 2);
  BOOST_TEST(loc->second->back().row_groups_ == Index::matching_groups{4});

  // Also should not overlap:
  insert(13, 20, 5);
  BOOST_TEST(loc->second->size() == 3);
  BOOST_TEST(loc->second->back().row_groups_ == Index::matching_groups{5});
}
