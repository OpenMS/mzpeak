/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <any>
#include <functional>
#include <type_traits>
#include <variant>

#include "mzpeak/util/struct.h"

namespace MzPeak {

template <typename T>
concept query_comparable = std::same_as<std::remove_cvref_t<T>, int32_t> ||
                           std::same_as<std::remove_cvref_t<T>, int64_t> ||
                           std::same_as<std::remove_cvref_t<T>, float> ||
                           std::same_as<std::remove_cvref_t<T>, double>;

/**
 * A low-level interface for selecting which records to extract from a
 * Parquet file.
 *
 * Queries are built using the Builder class and one of the provided
 * predicate functions.
 *
 * More complex queries can be constructed using the logic operators
 * (`&&`, `||`, and `!`).  NOTE: Keep in mind that complex queries are
 * built and evaluated using recursion so depth should be kept to a
 * minimum.
 */
class Query final {
public:
  using Struct = Util::Struct;
  using Field = Util::Struct::Field;
  struct Predicate;

  // How to specify what field you want to query.
  using destination_t =
      std::pair<std::shared_ptr<const Struct>, std::shared_ptr<const Field>>;

  /**
   * This class is used to construct a Query object using two inputs:
   *
   * 1. A Parquet struct and field to compare to (`destination_t`)
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
    Builder(destination_t destination)
        : dest_(destination)
    {
    }

    /**
     * Field must match `val` exactly.
     */
    template <query_comparable T> Query eq(T val) const
    {
      return validate({dest_, Predicate::Op::EQ, val});
    }

    /**
     * Field must be greater than `val`.
     */
    template <query_comparable T> Query gt(T val) const
    {
      return validate({dest_, Predicate::Op::GT, val});
    }

    /**
     * Field must be less than `val`.
     */
    template <query_comparable T> Query lt(T val) const
    {
      return validate({dest_, Predicate::Op::LT, val});
    }

    /**
     * Field must be greater than or equal to `val`.
     */
    template <query_comparable T> Query ge(T val) const
    {
      return validate({dest_, Predicate::Op::GE, val});
    }

    /**
     * Field must be less than or equal to `val`.
     */
    template <query_comparable T> Query le(T val) const
    {
      return validate({dest_, Predicate::Op::LE, val});
    }

    /// Destructor.
    ~Builder() = default;

  private:
    Query validate(Predicate&& p) const;
    destination_t dest_;
  };

public:
  /// Destructor.
  ~Query();

  /// Join two queries together with a logical AND.
  Query operator&&(const Query&) const;

  /// Join two queries together with a logical OR.
  Query operator||(const Query&) const;

  /// Negate a query.
  Query operator!() const;

  // Internal boolean operator type.
  enum class Oper { AND, OR };

  // Internal type for recursion.
  struct child_t {
    Oper oper_;
    std::any lhs_;
    std::any rhs_;
  };

  using value_t = std::variant<int32_t, int64_t, float, double>;

  using range_t = std::variant<std::pair<int32_t, int32_t>,
                               std::pair<int64_t, int64_t>,
                               std::pair<float, float>,
                               std::pair<double, double>>;

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

    /// Convert to another type while preserving failure and NULL
    /// status.  That is, U is ignored if the current result is NULL.
    template <typename U> Result<U> to(U) const;

    /// Combine values using logical operations.
    Result operator&&(const Result& other);
    Result operator||(const Result& other);
    Result operator!();

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

  /// A function that when given an column type, should return a single value.
  /// If this isn't possible it should return nullopt.
  using eval_callback_t = std::function<Result<value_t>(destination_t)>;

  /// A func ion that when given an column type should return a min and
  /// max.  If this isn't possible it should return nullopt.
  using eval_range_callback_t = std::function<Result<range_t>(destination_t)>;

  /**
   * Evaluate a query.
   */
  Result<bool> eval(eval_callback_t) const;

  /**
   * Evaluate a range query.
   *
   * Range queries test to see if the query would match a value within
   * a min or max range.
   */
  Result<bool> eval(eval_range_callback_t) const;

  // Internal predicate details.
  struct Predicate {
    enum class Op { EQ, GT, LT, GE, LE };
    destination_t dest;
    Op op;
    value_t val;
  };

private:
  friend class Builder;

  std::optional<Predicate> self_;
  std::optional<child_t> child_;
  bool not_ = false;

  Query(Predicate pred);
  explicit Query(const child_t&);
  Query join(const Query& other, Oper oper) const;
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
Query::Result<T> Query::Result<T>::operator&&(const Query::Result<T>& other)
{
  if (failed_) return *this;
  if (other.failed_) return other;
  if (!result_.has_value()) return *this;
  if (!other.result_.has_value()) return other;
  return Result(result_.value() && other.result_.value());
}

/******************************************************************************/
template <typename T>
Query::Result<T> Query::Result<T>::operator||(const Query::Result<T>& other)
{
  if (failed_) return *this;
  if (other.failed_) return other;
  if (!result_.has_value()) return other;
  if (!other.result_.has_value()) return *this;
  return Result(result_.value() || other.result_.value());
}

/******************************************************************************/
template <typename T> Query::Result<T> Query::Result<T>::operator!()
{
  Result r = *this;
  r.result_ = r.result_.and_then([](auto& v) -> std::optional<T> { return !v; });
  return r;
}

} // namespace MzPeak
