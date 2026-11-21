/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <any>
#include <boost/compat/function_ref.hpp>
#include <variant>

#include "mzpeak/schema/group.h"
#include "mzpeak/util/plan.h"
#include "mzpeak/util/types.h"

namespace MzPeak::Util {

/**
 * A low-level interface for selecting which records to extract from a
 * Parquet file.
 *
 * Queries are built using the Builder class and one of the provided
 * predicate functions.
 *
 * More complex queries can be constructed using the logic operators
 * (`&&`, `||`, and `!`) using the `and_then`, `or_else`, and `negate`
 * functions.
 */
class Query final {
public:
  // A predicate that can be tested against a value.
  struct Predicate final {
    enum Op { EQ, GT, LT, GE, LE };
    Schema::Column dest;
    Op op;
    any_value_type val;
  };

  /**
   * This class is used to construct a Query object using two inputs:
   *
   * 1. A Parquet group and field to compare to (`Schema::Column`)
   *
   * 2. A predicate function with a comparison value.
   *
   * NOTE: The C++ type given to the predicate functions (e.g., `eq`,
   * `gt`) must be compatible with the field type.  Currently this
   * is rather strict.  For example, if the field type is a PSI Int32,
   * then the predicate value *must* be an `int32_t`.
   *
   * Unfortunately this can only be checked at run-time without making
   * the query interface very difficult to use.  Therefore, mismatches
   * are reported as run-time exceptions.
   */
  class Builder final {
  public:
    /// Constructor.
    Builder(Schema::Column destination)
        : dest_(destination)
    {
    }

    /**
     * Field must match `val` exactly.
     */
    template <supported_type T> Query eq(T val) const
    {
      return validate({dest_, Predicate::EQ, val});
    }

    /**
     * Field must be greater than `val`.
     */
    template <supported_type T> Query gt(T val) const
    {
      return validate({dest_, Predicate::GT, val});
    }

    /**
     * Field must be less than `val`.
     */
    template <supported_type T> Query lt(T val) const
    {
      return validate({dest_, Predicate::LT, val});
    }

    /**
     * Field must be greater than or equal to `val`.
     */
    template <supported_type T> Query ge(T val) const
    {
      return validate({dest_, Predicate::GE, val});
    }

    /**
     * Field must be less than or equal to `val`.
     */
    template <supported_type T> Query le(T val) const
    {
      return validate({dest_, Predicate::LE, val});
    }

    /**
     * Low-level function for build a query with an operator.
     */
    template <supported_type T> Query via(T val, Predicate::Op op) const
    {
      return validate({dest_, op, val});
    }

  private:
    Query validate(Predicate&& p) const;
    Schema::Column dest_;
  };

public:
  /**
   * A class used to return values to the query engine, and also the
   * final result return from query evaluation.
   *
   * This class models three possible states:
   *
   * 1. Failure.  The query should be terminated.
   *
   * 2. Null.  Treated like SQL NULL values.  That is, not an error
   * but might cause queries to short circuit.  Useful to signal that
   * certain columns can't be read because they are NULL.
   *
   * 3. Contains a valid value.
   */
  template <typename T> class Result {
  public:
    /// Unrecoverable failure.
    static Result fail() { return Result(true, std::nullopt); }

    /// NULL.
    static Result skip() { return Result(false, std::nullopt); }

    /// Valid value.
    Result(T);

    /// Did the column request fail?
    bool failed() const { return failed_; }

    /// Not failed and not NULL.
    bool has_value() const;

    /// Get the actual value recorded.
    T value() const;

    /// Return true if the result is not failed, not skipped, has a
    /// value, and that value is the given value.
    bool is(T) const;

    /// See `Result.is`.
    explicit operator bool() const noexcept;

    /// Convert to another type while preserving failure and NULL
    /// status.  That is, U is ignored if the current result is NULL.
    template <typename U> Result<U> to(U) const;

    /// Combine values using logical operations.
    Result and_then(const Result& other);
    Result or_else(const Result& other);
    Result negate();

  private:
    explicit Result(bool f, std::optional<T> r)
        : failed_(f)
        , result_(r)
    {
    }

    // This is so stupid.
    template <typename U> friend class Result;

    bool failed_;
    std::optional<T> result_;
  };

public:
  /// Join two queries together with a logical AND.
  Query and_then(const Query&) const;

  /// Join two queries together with a logical OR.
  Query or_else(const Query&) const;

  /// Negate a query.
  Query negate() const;

  // Column types that can be used in a query.
  using value_t = any_value_type; // From types.h

  /// A function that when given an column type, should return a single value.
  /// If this isn't possible it should return `Result<value_t>::skip()`.
  using eval_callback_t =
      boost::compat::function_ref<Result<value_t>(Schema::Column)>;

  /// A function that when given a `Predicate` returns a `Plan` that
  /// can be used to fetch rows to fulfill that predicate.
  using plan_callback_t =
      boost::compat::function_ref<Result<Plan>(const Predicate&)>;

  /**
   * Evaluate a query.
   */
  Result<bool> eval(eval_callback_t) const;

  /**
   * Evaluate a query and return a Plan.
   */
  std::optional<Plan> plan(plan_callback_t) const;

private:
  friend class Builder;
  template <typename T, typename Fn> friend struct Visitor;

  // A logical node with a connective.
  struct Node {
    enum class Connective { AND, OR };
    Connective connective_;
    std::any lhs_;
    std::any rhs_;
  };

  // A tree is a leaf or a node.
  using tree_t = std::variant<Predicate, Node>;

  // Private constructors.
  explicit Query(Predicate&& pred);
  explicit Query(Node&&);

  // Private helper function.
  Query join(const Query& other, Node::Connective conn) const;

  // Data members.
  tree_t tree_;
  bool not_ = false;
};

/******************************************************************************/
template <typename T>
Query::Result<T>::Result(T v)
    : failed_(false)
    , result_(v)
{
}

/******************************************************************************/
template <typename T> bool Query::Result<T>::has_value() const
{
  return !failed_ && result_.has_value();
}

/******************************************************************************/
template <typename T> T Query::Result<T>::value() const { return result_.value(); }

/******************************************************************************/
template <typename T> bool Query::Result<T>::is(T t) const
{
  if (failed_) return false;
  return has_value() && value() == t;
}

/******************************************************************************/
template <typename T> Query::Result<T>::operator bool() const noexcept
{
  if constexpr (std::is_same_v<T, bool>) {
    return is(true);
  } else if constexpr (requires(T t) { t.operator bool(); }) {
    if (failed_) return false;
    return has_value() && value();
  } else {
    static_assert(false_type<T>, "type has no boolean operator");
  }
}

/******************************************************************************/
template <typename T>
template <typename U>
Query::Result<U> Query::Result<T>::to(U u) const
{
  std::optional<U> r;
  if (has_value()) r = u;
  return Result<U>(failed_, r);
}

/******************************************************************************/
template <typename T>
Query::Result<T> Query::Result<T>::and_then(const Query::Result<T>& other)
{
  if (failed_) return *this;
  if (other.failed_) return other;
  if (!result_.has_value()) return *this;
  if (!other.result_.has_value()) return other;

  if constexpr (std::is_same_v<T, bool>) {
    return Result(result_.value() && other.result_.value());
  } else if constexpr (requires(T a, T b) { a.logical_and(b)->T; }) {
    return Result(result_.value().logical_and(other.result_.value()));
  } else {
    static_assert(false_type<T>, "type has no logical_and method");
  }
}

/******************************************************************************/
template <typename T>
Query::Result<T> Query::Result<T>::or_else(const Query::Result<T>& other)
{
  if (failed_) return *this;
  if (other.failed_) return other;
  if (!result_.has_value()) return other;
  if (!other.result_.has_value()) return *this;

  if constexpr (std::is_same_v<T, bool>) {
    return Result(result_.value() || other.result_.value());
  } else if constexpr (requires(T a, T b) { a.logical_or(b)->T; }) {
    return Result(result_.value().logical_or(other.result_.value()));
  } else {
    static_assert(false_type<T>, "type has no logical_or method");
  }
}

/******************************************************************************/
template <typename T> Query::Result<T> Query::Result<T>::negate()
{
  Result r = *this;

  r.result_ = r.result_.and_then([](auto& v) -> std::optional<T> {
    if constexpr (std::is_same_v<T, bool>) {
      return !v;
    } else if constexpr (requires(T a) { a.logical_not(); }) {
      return v.logical_not();
    } else {
      static_assert(false_type<T>, "type has no logical_not method");
    }
  });

  return r;
}

} // namespace MzPeak::Util
