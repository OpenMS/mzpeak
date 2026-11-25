/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <cstdint>
#include <set>
#include <vector>

namespace MzPeak::Util {

/// NOTE: The Plan class uses set logic (∧=∩ and ∨=∪) over the row
/// groups that need to be fetched.  The `All` case serves two
/// purposes, 1) mark that a full scan is needed, and 2) avoid
/// materializing a set containing all of the row group IDs.
class Plan final {
public:
  // A set of row group IDs.
  using rgs_t = std::set<int32_t>;

  enum Type {
    /// Full table scan required.
    All,

    /// Some row groups match.
    Some,
    //
    /// All row groups except these.
    Except,
  };

  /// Construct a plan that requires a full table scan.
  static Plan full(int32_t total) { return Plan(All, {}, total); }

  /// Constructor.
  explicit Plan(int32_t number_of_row_groups);

  /// Constructor
  explicit Plan(Type, rgs_t, int32_t);

  /**
   * Return `true` if the plan requires a full table scan.
   */
  bool is_full_scan() const;

  /**
   * Return a list of row groups that are needed.
   *
   * NOTE: if the plan requires a full table scan this function can
   * return a possibly large set.  Prefer using `is_full_scan()`
   * first.
   */
  rgs_t materalize() const;

  /// Same as `materalize` except a vector is returned.
  std::vector<int32_t> materalize_as_vector() const;

  /// Combine the given plan with this plan using logical AND.
  void inplace_and(const Plan&);

  /// Combine the given plan with this plan using logical OR.
  void inplace_or(const Plan&);

  /// Negate this plan.
  void inplace_not();

  /// Return a new Plan that is the combination of this plan and the
  /// given plan using a logical AND.
  Plan logical_and(const Plan&) const;

  /// Return a new Plan that is the combination of this plan and the
  /// given plan using a logical OR.
  Plan logical_or(const Plan&) const;

  /// Return the logical negation of this plan.
  Plan logical_not() const;

private:
  Type type_;
  int32_t num_row_groups_;
  rgs_t row_groups_;
};

} // namespace MzPeak::Util
