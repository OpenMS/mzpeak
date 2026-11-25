/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <cassert>
#include <functional>
#include <utility>
#include <variant>

#include "mzpeak/exception.h"
#include "mzpeak/util/compat.h" // IWYU pragma: keep
#include "mzpeak/util/query.h"

namespace MzPeak::Util {

/// Classic trampoline to turn recursive algorithms into iteration.
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
// Walk the query tree and call a function on each predicate.
template <typename T, typename Fn> struct Visitor {
  using result_t = Query::Result<T>;

  //
  Trampoline<result_t> walk(const Query& query, Fn fn) const;

  // Eval a query node.
  Trampoline<result_t> visit_node(const Query::Node& node, Fn fn) const;

  // Dispatch on the type of the predicate's value.
  result_t visit_predicate(const Query::Predicate& p, Fn fn) const;
};

/******************************************************************************/
template <typename T, typename Fn>
Trampoline<typename Visitor<T, Fn>::result_t>
Visitor<T, Fn>::walk(const Query& query, Fn fn) const
{
  result_t result = std::visit(
      [&](auto& tree) -> result_t {
        using U = std::decay_t<decltype(tree)>;

        if constexpr (std::is_same_v<U, Query::Predicate>) {
          return visit_predicate(tree, fn);
        } else if constexpr (std::is_same_v<U, Query::Node>) {
          return trampoline(visit_node(tree, fn));
        } else {
          static_assert(false_type<U>, "unhanded variant");
        }
      },
      query.tree_);

  if (query.not_) {
    return result.negate();
  } else {
    return result;
  }
}

/******************************************************************************/
template <typename T, typename Fn>
Trampoline<typename Visitor<T, Fn>::result_t>
Visitor<T, Fn>::visit_node(const Query::Node& node, Fn fn) const
{
  // Should be impossible:
  assert(node.lhs_.has_value());
  result_t result(trampoline(walk(std::any_cast<Query>(node.lhs_), fn)));

  constexpr bool can_short_circuit = std::is_same_v<T, bool>;

  if (node.rhs_.has_value()) {
    switch (node.connective_) {
    case Query::Node::Connective::AND:
      if constexpr (can_short_circuit) {
        if (!result.is(true)) return result;
      }

      return Trampoline(trampoline(walk(std::any_cast<Query>(node.rhs_), fn)));
    case Query::Node::Connective::OR:
      if constexpr (can_short_circuit) {
        if (result.is(true)) return result;
      }

      return Trampoline(trampoline(walk(std::any_cast<Query>(node.rhs_), fn)));
    }
  }

  return result;
}

/******************************************************************************/
template <typename T, typename Fn>
Visitor<T, Fn>::result_t Visitor<T, Fn>::visit_predicate(const Query::Predicate& p,
                                                         Fn fn) const
{
  if (!p.dest.second->type().has_value()) {
    std::string msg("invalid query on field with unknown data type: ");
    msg += p.dest.first->path(*p.dest.second);
    throw TypeError(msg);
  }

  return std::invoke(fn, p);
}

/******************************************************************************/
Query Query::Builder::validate(Query::Predicate&& p) const
{
  if (!p.dest.second->type().has_value()) {
    std::string msg("cannot query field with unknown type: ");
    msg += p.dest.first->path(*p.dest.second);
    throw TypeError(msg);
  }

  std::visit([&type = p.dest.second->type().value()](
                 auto&& v) -> void { ensure_same_type<decltype(v)>(type); },
             p.val);

  return Query(std::move(p));
}

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
Query Query::negate() const
{
  Query q(*this);
  q.not_ = !q.not_;
  return q;
}

/******************************************************************************/
Query Query::and_(const Query& rhs) const
{
  return join(rhs, Node::Connective::AND);
}

/******************************************************************************/
Query Query::or_(const Query& rhs) const { return join(rhs, Node::Connective::OR); }

/******************************************************************************/
Query Query::join(const Query& other, Node::Connective oper) const
{
  return Query(Node{oper, *this, other});
}

/******************************************************************************/
Query::Result<bool> Query::eval(eval_callback_t fn) const
{
  auto bridge = [&fn](const Query::Predicate& p) -> Query::Result<bool> {
    auto res = std::invoke(fn, p.dest);

    if (!res.has_value()) {
      return res.template to<bool>(false);
    } else {
      auto v = res.value();
      assert(v.index() == p.val.index());

      switch (p.op) {
      case Query::Predicate::EQ:
        return v == p.val;

      case Query::Predicate::GT:
        return v > p.val;

      case Query::Predicate::LT:
        return v < p.val;

      case Query::Predicate::GE:
        return v >= p.val;

      case Query::Predicate::LE:
        return v <= p.val;
      }
    }

    std::unreachable();
  };

  Visitor<bool, decltype(bridge)> visitor;
  return trampoline(visitor.walk(*this, bridge));
}

/******************************************************************************/
std::optional<Plan> Query::plan(plan_callback_t fn) const
{
  Visitor<Plan, plan_callback_t> visitor;
  auto result = trampoline(visitor.walk(*this, fn));

  if (result.has_value()) {
    return result.value();
  } else {
    return std::nullopt;
  }
}

} // namespace MzPeak::Util
