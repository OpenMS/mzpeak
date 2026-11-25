/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <algorithm>
#include <iterator>
#include <ranges>
#include <utility>

#include "mzpeak/util/plan.h"

namespace MzPeak::Util {

/******************************************************************************/
Plan::Plan(int32_t number_of_row_groups)
    : type_(Some)
    , num_row_groups_(number_of_row_groups)
    , row_groups_()
{
}

/******************************************************************************/
Plan::Plan(Type t, rgs_t r, int32_t num)
    : type_(t)
    , num_row_groups_(num)
    , row_groups_(std::move(r))
{
}

/******************************************************************************/
bool Plan::is_full_scan() const { return type_ == All; }

/******************************************************************************/
Plan::rgs_t Plan::materalize() const
{
  auto all = [&] {
    return std::views::iota(0, num_row_groups_) | std::ranges::to<std::set>();
  };

  switch (type_) {
  case All:
    return all();
  case Some:
    return row_groups_;
  case Except: {
    rgs_t res{};
    std::ranges::set_difference(all(), row_groups_, std::inserter(res, res.end()));
    return res;
  }
  }

  std::unreachable();
}

/******************************************************************************/
std::vector<int32_t> Plan::materalize_as_vector() const
{
  rgs_t set = materalize();
  std::vector<int32_t> res(set.begin(), set.end());
  return res;
}

/******************************************************************************/
void Plan::inplace_and(const Plan& other)
{
  if (type_ == All) {
    // Full scans disappear due to set intersection.
    type_ = other.type_;
    row_groups_ = other.row_groups_;
  } else if (other.type_ == All) {
    return;
  } else {
    type_ = Some;
    // a ∧ b = a ∩ b
    rgs_t r{};
    std::ranges::set_intersection(materalize(), other.materalize(),
                                  std::inserter(r, r.end()));
    std::swap(row_groups_, r);
  }
}

/******************************************************************************/
void Plan::inplace_or(const Plan& other)
{
  if (type_ == All) {
    // Full scans remain due to set union.
    return;
  } else if (other.type_ == All) {
    type_ = other.type_;
    row_groups_.clear();
    return;
  } else {
    // a ∨ b = a ∪ b;
    type_ = Some;
    rgs_t r{};
    r.insert_range(materalize());
    r.insert_range(other.materalize());
    std::swap(row_groups_, r);
  }
}

/******************************************************************************/
void Plan::inplace_not()
{
  switch (type_) {
  case All:
    // If you have to do a full table scan to satisfy a query, you
    // have to do a full table scan to satisfy its negation.
    break;
  case Some:
    type_ = Except;
    break;
  case Except:
    type_ = Some;
    break;
  }
}

/******************************************************************************/
Plan Plan::logical_and(const Plan& other) const
{
  Plan res(*this);
  res.inplace_and(other);
  return res;
}

/******************************************************************************/
Plan Plan::logical_or(const Plan& other) const
{
  Plan res(*this);
  res.inplace_or(other);
  return res;
}

/******************************************************************************/
Plan Plan::logical_not() const
{
  Plan res(*this);
  res.inplace_not();
  return res;
}

} // namespace MzPeak::Util
