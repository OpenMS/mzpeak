/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE UtilPlan
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/util/plan.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(plan_logic)
{
  using namespace MzPeak::Util;
  const int32_t rgs = 10;

  auto all = [&] { return Plan::full(rgs); };
  auto some = [&](Plan::rgs_t&& set) { return Plan(Plan::Some, set, rgs); };

  auto assert_plan = [](const Plan& p, const Plan::rgs_t&& s) {
    BOOST_TEST((p.materalize() == s));
  };

  BOOST_TEST((all().materalize().size() == rgs));

  Plan base(some({0}));
  Plan half(some({0, 1, 2, 3, 4}));

  assert_plan(base.logical_or(some({1})), {0, 1});
  assert_plan(base.logical_and(some({1})), {});
  assert_plan(half.logical_not(), {5, 6, 7, 8, 9});
  assert_plan(half.logical_and(half.logical_not()), {});
  assert_plan(half.logical_or(half.logical_not()), all().materalize());
  assert_plan(all().logical_and(some({0})), {0});
  assert_plan(some({0}).logical_and(all()), {0});
  assert_plan(all().logical_or(some({0})), all().materalize());
  assert_plan(some({0}).logical_or(all()), all().materalize());
}
