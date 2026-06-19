/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <cassert>
#include <functional>
#include <variant>

#include "mzpeak/exception.h"
#include "mzpeak/query.h"
#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/util/compat.h" // IWYU pragma: keep

namespace MzPeak {

/**
 * Classic trampoline to turn recursive algorithms into iteration.
 */
template <typename T> struct Trampoline {
  using thunk_t = std::move_only_function<Trampoline<T>()>;

  Trampoline(T&& v)
      : value_(std::move(v))
  {
  }

  Trampoline(thunk_t&& t)
      : value_(std::move(t))
  {
  }

  std::variant<T, thunk_t> value_;
};

/******************************************************************************/
template <typename T> inline T trampoline(Trampoline<T> t)
{
  while (!std::holds_alternative<T>(t.value_)) {
    t = std::invoke(std::get<typename Trampoline<T>::thunk_t>(t.value_));
  }

  return std::get<T>(t.value_);
}

/******************************************************************************/
// Helper to produce useful messages with static_assert.
template <typename...> inline constexpr bool failed_match = false;

/******************************************************************************/
std::pair<Query::value_t, Query::value_t> decode_range_type(const Query::range_t& rt)
{
  return std::visit(
      [](auto&& pair) {
        auto first = Query::value_t{pair.first};
        auto second = Query::value_t{pair.second};
        return std::make_pair(first, second);
      },
      rt);
}

/******************************************************************************/
Query Query::Builder::validate(Query::Predicate&& p) const
{
  if (!p.dest.second->data_type().has_value()) {
    std::string msg("cannot query field with unknown type: ");
    msg += p.dest.first->name() + "." + p.dest.second->name();
    throw TypeError(msg);
  }

  std::visit(
      [p](auto&& v) -> void {
        using T = std::decay_t<decltype(v)>;

        Schema::PSI::DataType query_type =
            Schema::PSI::data_type_for_value_type<T>();
        Schema::PSI::DataType field_type = p.dest.second->data_type().value();

        if (field_type != query_type) {
          std::string msg("query type does not match field type for ");
          msg += p.dest.first->name() + "." + p.dest.second->name() + ": ";
          msg += Schema::PSI::data_type_to_string(query_type) + " != ";
          msg += Schema::PSI::data_type_to_string(field_type);
          throw TypeError(msg);
        }
      },
      p.val);

  return Query(std::move(p));
}

/******************************************************************************/
template <typename Fn, typename V> struct EvalHelper {
  using result_t = Query::Result<bool>;

  // Eval a query using the given function for fetching values.
  Trampoline<Query::Result<bool>> eval(const Query& query, Fn fn) const;

  // Eval a query node.
  Trampoline<Query::Result<bool>> eval_node(const Query::Node& node, Fn fn) const;

  // Match a predicate against a single value.
  bool match(const Query::Predicate&, Query::value_t) const;

  // Match a predicate against a min/max range.
  bool match(const Query::Predicate& p, Query::range_t) const;

  // Dispatch on the type of the given predicate.
  Query::Result<bool> dispatch_dest_type(const Query::Predicate& pred, Fn fn) const;

  // Dispatch on the type of the predicate's value.
  template <Schema::PSI::DataType T>
  Query::Result<bool> dispatch_value(const Query::Predicate& p, Fn fn) const;
};

/******************************************************************************/
Query::Query(Predicate&& p)
    : tree_({std::move(p)})
{
}

/******************************************************************************/
Query::Query(Node&& c)
    : tree_({std::move(c)})
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
Query Query::operator&&(const Query& rhs) const
{
  return join(rhs, Node::Connective::AND);
}

/******************************************************************************/
Query Query::operator||(const Query& rhs) const
{
  return join(rhs, Node::Connective::OR);
}

/******************************************************************************/
Query Query::join(const Query& other, Node::Connective oper) const
{
  return Query(Node{oper, *this, other});
}

/******************************************************************************/
Query::Result<bool> Query::eval(eval_callback_t fn) const
{
  EvalHelper<eval_callback_t, value_t> eh;
  return trampoline(eh.eval(*this, fn));
}

/******************************************************************************/
Query::Result<bool> Query::eval(eval_range_callback_t fn) const
{
  EvalHelper<eval_range_callback_t, range_t> eh;
  return trampoline(eh.eval(*this, fn));
}

/******************************************************************************/
template <typename Fn, typename V>
bool EvalHelper<Fn, V>::match(const Query::Predicate& p, Query::value_t v) const
{
  assert(v.index() == p.val.index());

  switch (p.op) {
  case Query::Op::EQ:
    return v == p.val;

  case Query::Op::GT:
    return v > p.val;

  case Query::Op::LT:
    return v < p.val;

  case Query::Op::GE:
    return v >= p.val;

  case Query::Op::LE:
    return v <= p.val;
  }

  return false;
}

/******************************************************************************/
template <typename Fn, typename V>
bool EvalHelper<Fn, V>::match(const Query::Predicate& p, Query::range_t v) const
{
  auto [min, max] = decode_range_type(v);
  assert(min.index() == max.index() && min.index() == p.val.index());

  switch (p.op) {
  case Query::Op::EQ:
    return (min == p.val || max == p.val) || (p.val > min && p.val < max);

  case Query::Op::GT:
    return max > p.val;

  case Query::Op::LT:
    return min < p.val;

  case Query::Op::GE:
    return max >= p.val;

  case Query::Op::LE:
    return min <= p.val;
  }

  return false;
}

/******************************************************************************/
template <typename Fn, typename V>
template <Schema::PSI::DataType T>
Query::Result<bool> EvalHelper<Fn, V>::dispatch_value(const Query::Predicate& p,
                                                      Fn fn) const
{
  Query::Result<V> val(std::invoke(fn, p.dest));
  if (!val.has_value()) return val.template to<bool>(false);

  return std::visit(
      [&](auto&& v) -> Query::Result<bool> {
        using T1 = Schema::PSI::data_type_traits<T>::value_type;
        using T2 = std::decay_t<decltype(v)>;
        using T3 = std::pair<T1, T1>;

        if constexpr (std::is_same_v<T1, T2>) {
          return match(p, v);
        } else if constexpr (std::is_same_v<T2, T3>) {
          return match(p, v);
        } else {
          std::string msg("predicate and value mismatch: ");
          throw TypeError(msg);
          return Query::Result<bool>::fail(); // clang is too stupid to see the throw
        }
      },
      val.value());
}

/******************************************************************************/
template <typename Fn, typename V>
Query::Result<bool>
EvalHelper<Fn, V>::dispatch_dest_type(const Query::Predicate& pred, Fn fn) const
{
  if (!pred.dest.second->data_type().has_value()) {
    std::string msg("invalid query on field with unknown data type: ");
    msg += pred.dest.first->name() + "." + pred.dest.second->name();
    throw TypeError(msg);
  }

  using enum Schema::PSI::DataType;

  switch (pred.dest.second->data_type().value()) {
  case Int32:
    return dispatch_value<Int32>(pred, fn);
  case Int64:
    return dispatch_value<Int64>(pred, fn);
  case Float32:
    return dispatch_value<Float32>(pred, fn);
  case Float64:
    return dispatch_value<Float64>(pred, fn);
  case ASCII: {
    std::string msg("unsupported ASCII query on: ");
    msg += pred.dest.first->name() + "." + pred.dest.second->name();
    throw TypeError(msg);
  }
  }

  return false;
}

/******************************************************************************/
template <typename Fn, typename V>
Trampoline<Query::Result<bool>> EvalHelper<Fn, V>::eval(const Query& query,
                                                        Fn fn) const
{
  result_t result = std::visit(
      [&](auto& tree) -> result_t {
        using T = std::decay_t<decltype(tree)>;

        if constexpr (std::is_same_v<T, Query::Predicate>) {
          return dispatch_dest_type(tree, fn);
        } else if constexpr (std::is_same_v<T, Query::Node>) {
          return trampoline(eval_node(tree, fn));
        } else {
          static_assert(false, "unhanded variant");
        }
      },
      query.tree_);

  if (query.not_) {
    return !result;
  } else {
    return result;
  }
}

/******************************************************************************/
template <typename Fn, typename V>
Trampoline<Query::Result<bool>> EvalHelper<Fn, V>::eval_node(const Query::Node& node,
                                                             Fn fn) const
{
  result_t result = true;

  if (node.lhs_.has_value()) {
    result = trampoline(eval(std::any_cast<Query>(node.lhs_), fn));
  }

  if (node.rhs_.has_value()) {
    switch (node.connective_) {
    case Query::Node::Connective::AND:
      if (result.is(true)) {
        return Trampoline(trampoline(eval(std::any_cast<Query>(node.rhs_), fn)));
      }
    case Query::Node::Connective::OR:
      if (result.is(false)) {
        return Trampoline(trampoline(eval(std::any_cast<Query>(node.rhs_), fn)));
      }
    }
  }

  return result;
}

} // namespace MzPeak
