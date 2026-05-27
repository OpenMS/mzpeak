/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include "mzpeak/query.h"
#include "mzpeak/exception.h"
#include "mzpeak/schema/psi/data_type.h"
#include <type_traits>

namespace MzPeak {

/******************************************************************************/
// Helper to produce useful messages with static_assert.
template <typename...> inline constexpr bool failed_match = false;

/******************************************************************************/
template <typename Fn, typename V> struct EvalHelper {

  // Eval a query using the given function for fetching values.
  bool eval(Fn fn) const;

  // Dispatch on the type of the given predicate.
  bool dispatch_pred(const Query::predicate_t& pred, Fn fn) const;

  // Dispatch on the type of the predicate's value.
  template <Schema::PSI::DataType T>
  bool dispatch_value(const Query::Predicate<T>& p, Fn fn) const;

  // From the query being evaluated:
  const std::optional<Query::predicate_t>& self_;
  const std::optional<Query::child_t>& child_;
  bool not_;
};

/******************************************************************************/
Query::Query(predicate_t p)
    : self_(p)
{
}

/******************************************************************************/
Query::Query(const child_t& c)
    : child_(c)
{
}

/******************************************************************************/
Query Query::operator!() const
{
  Query q(*this);
  q.not_ = !q.not_;
  return q;
}

/******************************************************************************/
Query::~Query() = default;

/******************************************************************************/
Query Query::operator&&(const Query& rhs) const { return join(rhs, Oper::AND); }

/******************************************************************************/
Query Query::operator||(const Query& rhs) const { return join(rhs, Oper::OR); }

/******************************************************************************/
Query Query::join(const Query& other, Oper oper) const
{
  return Query(child_t{oper, *this, other});
}

/******************************************************************************/
bool Query::eval(eval_callback_t fn) const
{
  EvalHelper<eval_callback_t, value_t> eh{self_, child_, not_};
  return eh.eval(fn);
}

/******************************************************************************/
bool Query::eval(eval_range_callback_t fn) const
{
  EvalHelper<eval_range_callback_t, range_t> eh{self_, child_, not_};
  return eh.eval(fn);
}

/******************************************************************************/
template <typename Fn, typename V>
template <Schema::PSI::DataType T>
bool EvalHelper<Fn, V>::dispatch_value(const Query::Predicate<T>& p, Fn fn) const
{
  std::optional<V> val(std::invoke(fn, p.column()));
  if (!val.has_value()) return false;

  return std::visit(
      [p](auto&& v) {
        using T1 = typename Query::Predicate<T>::value_type;
        using T2 = std::decay_t<decltype(v)>;
        using T3 = std::pair<T1, T1>;

        if constexpr (std::is_same_v<T1, T2>) {
          return p.match(v);
        } else if constexpr (std::is_same_v<T2, T3>) {
          return p.match(v);
        } else {
          std::string msg("predicate and value mismatch: ");
          msg += Schema::PSI::data_type_to_string(p.column().data_type);
          throw TypeError(msg);
          return false; // clang is too stupid to see the throw
        }
      },
      *val);
}

/******************************************************************************/
template <typename Fn, typename V>
bool EvalHelper<Fn, V>::dispatch_pred(const Query::predicate_t& pred, Fn fn) const
{
  using enum Schema::PSI::DataType;

  return std::visit(
      [&](auto&& v) {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, Query::Predicate<Int32>>) {
          return dispatch_value<Int32>(v, fn);
        } else if constexpr (std::is_same_v<T, Query::Predicate<Float32>>) {
          return dispatch_value<Float32>(v, fn);
        } else if constexpr (std::is_same_v<T, Query::Predicate<Int64>>) {
          return dispatch_value<Int64>(v, fn);
        } else if constexpr (std::is_same_v<T, Query::Predicate<Float64>>) {
          return dispatch_value<Float64>(v, fn);
        } else {
          static_assert(failed_match<T>, "unexpected predicate type");
        }
      },
      pred);
}

/******************************************************************************/
template <typename Fn, typename V> bool EvalHelper<Fn, V>::eval(Fn fn) const
{
  bool res = true;

  if (self_.has_value()) {
    res = dispatch_pred(*self_, fn);
  }

  if (child_.has_value()) {
    if (child_->lhs_.has_value()) {
      res = res && std::any_cast<Query>(child_->lhs_).eval(fn);
    }

    if (child_->rhs_.has_value()) {
      switch (child_->oper_) {
      case Query::Oper::AND:
        return res && std::any_cast<Query>(child_->rhs_).eval(fn);
      case Query::Oper::OR:
        return res || std::any_cast<Query>(child_->rhs_).eval(fn);
      }
    }
  }

  if (not_) {
    return !res;
  } else {
    return res;
  }
}

} // namespace MzPeak
