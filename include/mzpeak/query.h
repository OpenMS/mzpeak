/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/psi/data_type.h"

namespace MzPeak {

/**
 * FIXME:
 */
class Query {
public:
  template <Schema::PSI::DataType T> class Predicate final {
  public:
    ///
    using value_type = typename Schema::PSI::data_type_traits<T>::value_type;

    ///
    using array_type = Schema::ArrayIndex::Array;

    /**
     * Queried value must be exactly equal to the given value.
     */
    static Predicate<T> equal_to(const array_type& array, value_type v) {
      return Predicate(array, std::make_pair<>(Op::EQ, v));
    };

    /**
     * Queried value must be greater than the given value.
     */
    static Predicate<T> greater_than(const array_type& array, value_type v) {
      return Predicate(array, std::make_pair<>(Op::GT, v));
    };

    /**
     * Queried value must be less than the given value.
     */
    static Predicate<T> less_than(const array_type& array, value_type v) {
      return Predicate(array, std::make_pair<>(Op::LT, v));
    };

    /**
     * Queried value must be greater than or equal to the given value.
     */
    static Predicate<T> greater_equal(const array_type& array, value_type v) {
      return Predicate(array, std::make_pair<>(Op::GE, v));
    };

    /**
     * Queried value must be less than or equal to the given value.
     */
    static Predicate<T> less_equal(const array_type& array, value_type v) {
      return Predicate(array, std::make_pair<>(Op::LE, v));
    };

    /// Destructor.
    ~Predicate() = default;

    /**
     * The array this predicate works with.
     */
    const array_type& array() const { return array_; };

    /**
     * Return `true` if this predicate matches the given value.
     */
    bool match(value_type v) const;

    /**
     * Return `true` if the predicate would match a value in the range
     * (min, max).
     */
    bool match_in_range(value_type min, value_type max) const;

  private:
    /// Comparison operations.
    enum class Op { EQ, GT, LT, GE, LE };

    /// Complete description of the predicate.
    using Comp = std::pair<Op, value_type>;

    Predicate(const array_type& array, Comp comp)
        : array_(array), comp_(std::move(comp)) {};

    const array_type& array_;
    Comp comp_;
  };

public:
  /// Constructor.
  Query() {};

  /// Destructor.
  virtual ~Query() {};

  /**
   * Add a predicate to the list of predicates.
   */
  template <Schema::PSI::DataType T> void push_back(const Predicate<T>& p) {
    predicates_.push_back(p);
  };

public:
  //
  using enum Schema::PSI::DataType;

  /// FIXME
  using predicate_t = std::variant<Predicate<Int32>, Predicate<Float32>,
                                   Predicate<Int64>, Predicate<Float64>>;

  /**
   * FIXME
   */
  const std::vector<predicate_t> predicates() const { return predicates_; };

  /**
   * Extract an array from a predicate wrapper.
   */
  const Schema::ArrayIndex::Array& extract_array(const predicate_t& p) const {
    return *std::visit([](auto&& arg) { return &arg.array(); }, p);
  };

private:
  std::vector<predicate_t> predicates_;
};

/******************************************************************************/
template <Schema::PSI::DataType T>
bool Query::Predicate<T>::match(value_type v) const {
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
template <Schema::PSI::DataType T>
bool Query::Predicate<T>::match_in_range(value_type min, value_type max) const {
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
