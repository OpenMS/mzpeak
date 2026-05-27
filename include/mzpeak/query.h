/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <any>

#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/psi/data_type.h"

namespace MzPeak {

/**
 * A low-level interface for selecting which records to extract from a
 * Parquet file.
 *
 * Queries are built using one of the `Predicate<T>` functions along
 * with the array the predicate should match.
 *
 * More complex queries can be constructed using the logic operators
 * (`&&`, `||`, and `!`).  NOTE: Keep in mind that complex queries are
 * built and evaluated using recursion so depth should be kept to a
 * minimum.
 */
class Query {
public:
  template <Schema::PSI::DataType T> class Predicate final {
  public:
    /// The type of the value needed for this predicate.
    using value_type = typename Schema::PSI::data_type_traits<T>::value_type;

    /// The schema array type.
    using column_type = Schema::ArrayIndex::Column;

    /**
     * Queried value must be exactly equal to the given value.
     */
    static Query equal_to(const column_type& column, value_type v)
    {
      return Query(Predicate(column, std::make_pair<>(Op::EQ, v)));
    }

    /**
     * Queried value must be greater than the given value.
     */
    static Query greater_than(const column_type& column, value_type v)
    {
      return Query(Predicate(column, std::make_pair<>(Op::GT, v)));
    }

    /**
     * Queried value must be less than the given value.
     */
    static Query less_than(const column_type& column, value_type v)
    {
      return Query(Predicate(column, std::make_pair<>(Op::LT, v)));
    }

    /**
     * Queried value must be greater than or equal to the given value.
     */
    static Query greater_equal(const column_type& column, value_type v)
    {
      return Query(Predicate(column, std::make_pair<>(Op::GE, v)));
    }

    /**
     * Queried value must be less than or equal to the given value.
     */
    static Query less_equal(const column_type& column, value_type v)
    {
      return Query(Predicate(column, std::make_pair<>(Op::LE, v)));
    }

    /// Destructor.
    ~Predicate() = default;

    /**
     * The column this predicate works with.
     */
    const column_type& column() const { return column_; }

    /**
     * Return `true` if this predicate matches the given value.
     */
    bool match(value_type v) const;

    /**
     * Return `true` if the predicate would match a value in the range
     * (min, max).
     */
    bool match(const std::pair<value_type, value_type>&) const;

  private:
    /// Comparison operations.
    enum class Op { EQ, GT, LT, GE, LE };

    /// Complete description of the predicate.
    using Comp = std::pair<Op, value_type>;

    Predicate(const column_type& column, Comp comp)
        : column_(column)
        , comp_(std::move(comp))
    {
    }

    const column_type& column_;
    Comp comp_;
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

public:
  // Save some typing.
  using enum Schema::PSI::DataType;

  // The C++ types that are used by predicates.
  using p_int32_t = Schema::PSI::data_type_traits<Int32>::value_type;
  using p_float32_t = Schema::PSI::data_type_traits<Float32>::value_type;
  using p_int64_t = Schema::PSI::data_type_traits<Int64>::value_type;
  using p_float64_t = Schema::PSI::data_type_traits<Float64>::value_type;

  /// A variant that can hold any predicate type.
  using predicate_t = std::variant<Predicate<Int32>,
                                   Predicate<Float32>,
                                   Predicate<Int64>,
                                   Predicate<Float64>>;

  /// A variant that can hold any predicate value type.
  using value_t = std::variant<p_int32_t, p_float32_t, p_int64_t, p_float64_t>;

  /// A variant that can hold a min and max value for range queries.
  using range_t = std::variant<std::pair<p_int32_t, p_int32_t>,
                               std::pair<p_float32_t, p_float32_t>,
                               std::pair<p_int64_t, p_int64_t>,
                               std::pair<p_float64_t, p_float64_t>>;

  /// A function that when given an column type, should return a single value.
  /// If this isn't possible it should return nullopt.
  using eval_callback_t =
      std::function<std::optional<value_t>(const Schema::ArrayIndex::Column&)>;

  /// A function that when given an column type should return a min and
  /// max.  If this isn't possible it should return nullopt.
  using eval_range_callback_t =
      std::function<std::optional<range_t>(const Schema::ArrayIndex::Column&)>;
  /**
   * Evaluate a query.
   */
  bool eval(eval_callback_t) const;

  /**
   * Evaluate a range query.
   *
   * Range queries test to see if the query would match a value within
   * a min or max range.
   */
  bool eval(eval_range_callback_t) const;

protected:
  /// Constructor.
  Query(predicate_t p);

  friend Predicate<Int32>;
  friend Predicate<Float32>;
  friend Predicate<Int64>;
  friend Predicate<Float64>;

private:
  std::optional<predicate_t> self_;
  std::optional<child_t> child_;
  bool not_ = false;

  explicit Query(const child_t&);
  Query join(const Query& other, Oper oper) const;
};

/******************************************************************************/
// Predicate matching the way you would expect.
template <Schema::PSI::DataType T>
bool Query::Predicate<T>::match(value_type v) const
{
  switch (comp_.first) {
  case Op::EQ:
    return v == comp_.second;
  case Op::GT:
    return v > comp_.second;

  case Op::LT:
    return v < comp_.second;

  case Op::GE:
    return v >= comp_.second;

  case Op::LE:
    return v >= comp_.second;
  }

  return false;
}

/******************************************************************************/
// Predicate range matching that returns true if the predicate would
// match a value that is between a min and max (inclusive).
template <Schema::PSI::DataType T>
bool Query::Predicate<T>::match(const std::pair<value_type, value_type>& v) const
{
  auto [min, max] = v;

  switch (comp_.first) {
  case Op::EQ:
    return (min == comp_.second || max == comp_.second) ||
           (comp_.second > min && comp_.second < max);
  case Op::GT:
    return max > comp_.second;

  case Op::LT:
    return min < comp_.second;

  case Op::GE:
    return max >= comp_.second;

  case Op::LE:
    return min >= comp_.second;
  }

  return false;
}

} // namespace MzPeak
