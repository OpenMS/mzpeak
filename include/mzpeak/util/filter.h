/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <arrow/compute/expression.h>
#include <optional>

#include "mzpeak/schema/group.h"

/******************************************************************************/
namespace MzPeak::Util {

/**
 * Helper class for building arrow compute expressions.
 */
class Filter final {
public:
  /**
   * Convert a column into a expression field reference.
   */
  static arrow::compute::Expression field(const Schema::Column&);

  /**
   * An expression where a column is *equal* to a literal value.
   */
  template <supported_type T> static Filter eq(const Schema::Column&, T&&);

  /**
   * An expression where a column is *greater than or equal* to a literal value.
   */
  template <supported_type T> static Filter ge(const Schema::Column&, T&&);

  /**
   * An expression where a column is *less than* a literal value.
   */
  template <supported_type T> static Filter lt(const Schema::Column&, T&&);

  /// Default constructor.
  Filter();

  /// Constructor
  explicit Filter(const arrow::compute::Expression&);

  /**
   * Combine this filter with another using logical AND.  Returns `*this`.
   */
  Filter& and_(const Filter&);

  /**
   * Access the expression.
   */
  std::optional<arrow::compute::Expression> expression() const;

private:
  enum Op {
    EQ,
    GE,
    LT,
  };

  // Create a filter from a column, operator, and expression.
  static Filter column_op(const Schema::Column&, Op, arrow::compute::Expression&&);

  std::optional<arrow::compute::Expression> expr_;
};

/******************************************************************************/
template <supported_type T>
Filter Filter::eq(const Schema::Column& column, T&& literal)
{
  namespace ac = arrow::compute;
  return column_op(column, EQ, ac::literal<T>(std::forward<T>(literal)));
}

/******************************************************************************/
template <supported_type T>
Filter Filter::ge(const Schema::Column& column, T&& literal)
{
  namespace ac = arrow::compute;
  return column_op(column, GE, ac::literal<T>(std::forward<T>(literal)));
}

/******************************************************************************/
template <supported_type T>
Filter Filter::lt(const Schema::Column& column, T&& literal)
{
  namespace ac = arrow::compute;
  return column_op(column, LT, ac::literal<T>(std::forward<T>(literal)));
}

} // namespace MzPeak::Util
