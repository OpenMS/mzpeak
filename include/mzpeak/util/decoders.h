/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <arrow/array.h>
#include <functional>
#include <memory>
#include <ranges>

#include "mzpeak/util/compat.h" // IWYU pragma: keep
#include "mzpeak/util/types.h"

namespace MzPeak::Util::Decoders {

/******************************************************************************/
/**
 * `C` is either a container holding values of `V`, or `C` and `V` are
 * both scalar values of the same type.
 */
template <typename C, typename V>
concept scalar_or_container_of =
    std::same_as<std::remove_cvref_t<C>, std::remove_cvref_t<V>> ||
    (std::ranges::range<C> && std::convertible_to<std::ranges::range_value_t<C>, V>);

/******************************************************************************/
/**
 * `T` is a type that has a `decode` function that can decode values
 * from an `arrow::Array` and place the result in `R`.  The `R` type
 * can be a container or scalar value.
 *
 * The `decode` function should return `true` to indicate it can
 * continue to decode values.  If it returns `false` the chunk
 * decoding will stop.
 */
template <typename T, typename R>
concept from_arrow_array =
    requires { typename T::value_type; } &&
    scalar_or_container_of<R, typename T::value_type> &&
    requires(T t, const std::shared_ptr<arrow::Array>& a, R& r) {
      { t.decode(a, r) } -> std::same_as<void>;
    };

/******************************************************************************/
/**
 * Return `true` if the given Arrow array is a `ListArray`.
 */
bool is_list_array(const std::shared_ptr<arrow::Array>&);

/******************************************************************************/
/**
 * A NULL decoder that always skips NULL values.
 */
template <typename T> struct NullSkip final {
  /// Skip this null;
  std::optional<T> operator()(int64_t) { return std::nullopt; }
};

/******************************************************************************/
/**
 * A NULL decoder that replaces NULL values with zero.
 */
template <typename T> struct NullToZero final {
  /// Replace the given NULL with zero.
  std::optional<T> operator()(int64_t)
  {
    T zero{};
    return zero;
  }
};

/******************************************************************************/
template <typename Child> struct Helper {

  // Helper function to push a value into a container, or when `V` and
  // `C` are both the same type do assignment.
  template <typename V, typename C>
    requires scalar_or_container_of<C, V>
  inline void push(C& dst, V&& v)
  {
    if constexpr (requires { dst.push_back(v); }) {
      dst.push_back(v);
    } else if constexpr (std::is_same_v<C, std::remove_cvref_t<V>>) {
      dst = std::move(v);
    } else {
      static_assert(false_type<C, V>, "bad destination");
    }
  }
};

/******************************************************************************/
/**
 * A decoder that produces a vector of scalar values.
 */
template <typename V, typename C = std::vector<V>, typename N = NullSkip<V>>
  requires scalar_or_container_of<C, V>
class Scalar final : Helper<Scalar<V, C, N>> {
public:
  /// The types of values this decoder can decode.
  using value_type = V;

  /// The range or scalar type.
  using range_type = C;

  /// The null decoder type.
  using null_decoder_type = N;

  /// Default constructor.
  Scalar()
      : null_decoder_({})
  {
  }

  /// Constructor.
  Scalar(const null_decoder_type& decoder)
      : null_decoder_(decoder)
  {
  }

  /// Destructor.
  ~Scalar() = default;

  /// Decoding function.
  void decode(const std::shared_ptr<arrow::Array>& src, C& dst)
  {
    using array_type = type_traits<enum_type_v<V>>::array_type;
    using array_ptr_type = std::shared_ptr<array_type>;

    array_ptr_type casted = std::static_pointer_cast<array_type>(src);

    if constexpr (requires { null_decoder_.chunk(casted); }) {
      null_decoder_.chunk(casted);
    }

    for (int64_t i : std::views::iota(0, casted->length())) {
      if (casted->IsNull(i)) {
        std::optional<V> value = std::invoke(null_decoder_, i);
        if (value.has_value()) this->push(dst, std::move(*value));
      } else {
        this->push(dst, std::move(casted->Value(i)));
      }
    }
  }

private:
  null_decoder_type null_decoder_;
};

/******************************************************************************/
/**
 * A decoder where array elements are lists.
 *
 * NULL `ListArray` elements, and NULL elements inside the
 * `ListArray` are skipped.
 */
template <typename V, typename C = std::vector<V>>
  requires Decoders::scalar_or_container_of<C, V>
class List final : Helper<List<V, C>> {
public:
  /// Decodes vectors of type T.
  using value_type = std::vector<V>;

  /// Constructor.
  List() {}

  /// Destructor.
  ~List() = default;

  /// Decoding function.
  void decode(const std::shared_ptr<arrow::Array>& src, C& dst)
  {
    if (!is_list_array(src)) {
      std::string msg("expected an arrow list array but found: ");
      msg += src->type()->name();
      throw TypeError(msg);
    }

    std::shared_ptr<arrow::ListArray> casted =
        std::static_pointer_cast<arrow::ListArray>(src);

    for (int64_t i : std::views::iota(0, casted->length())) {
      if (!casted->IsNull(i)) {
        std::shared_ptr<arrow::Array> values(casted->value_slice(i));
        value_type res;
        res.reserve(values->length());
        Scalar<V, C>().decode(values, res);
        this->push(dst, res);
      }
    }
  }
};

} // namespace MzPeak::Util::Decoders
