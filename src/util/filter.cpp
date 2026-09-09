/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/util/filter.h"

namespace MzPeak::Util {

/******************************************************************************/
arrow::compute::Expression Filter::field(const Schema::Column& column)
{
  if (column.first->is_root()) {
    return arrow::compute::field_ref(column.second->name());
  } else {
    return arrow::compute::field_ref(column.first->path(*column.second));
  }
}

/******************************************************************************/
Filter::Filter()
    : expr_({})
{
}

/******************************************************************************/
Filter::Filter(const arrow::compute::Expression& expr)
    : expr_(expr)
{
}

/******************************************************************************/
Filter& Filter::and_(const Filter& other)
{
  if (expr_.has_value() && other.expr_.has_value()) {
    expr_ = arrow::compute::and_(expr_.value(), other.expr_.value());
  } else {
    expr_ = expr_.or_else([&other] { return other.expr_; });
  }

  return *this;
}

/******************************************************************************/
std::optional<arrow::compute::Expression> Filter::expression() const
{
  return expr_;
}

} // namespace MzPeak::Util
