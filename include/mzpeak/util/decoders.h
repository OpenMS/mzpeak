/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <arrow/array.h>
#include <functional>
#include <ranges>

namespace MzPeak::Util::Decoders {

/******************************************************************************/
/**
 * Type traits for casting.
 */
template <typename T> struct cast_traits;

template <> struct cast_traits<int8_t> {
  using array_type = arrow::Int8Array;
};

template <> struct cast_traits<uint8_t> {
  using array_type = arrow::UInt8Array;
};

template <> struct cast_traits<int32_t> {
  using array_type = arrow::Int32Array;
};

template <> struct cast_traits<uint32_t> {
  using array_type = arrow::UInt32Array;
};

template <> struct cast_traits<int64_t> {
  using array_type = arrow::Int64Array;
};

template <> struct cast_traits<uint64_t> {
  using array_type = arrow::UInt64Array;
};

template <> struct cast_traits<float> {
  using array_type = arrow::FloatArray;
};

template <> struct cast_traits<double> {
  using array_type = arrow::DoubleArray;
};

template <> struct cast_traits<std::string> {
  using array_type = arrow::StringArray;
};

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
 * A function that knows what to do with null values.
 *
 * It is given the casted arrow array that contains the null value and
 * the index of the null value.  It should return a value to insert
 * into the array or nullopt to signal this value should be skipped.
 */
template <typename T>
using null_decoder = std::move_only_function<std::optional<T>(
    const std::shared_ptr<typename cast_traits<T>::array_type>&, int64_t)>;

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
      static_assert(false, "bad destination");
    }
  };
};

/******************************************************************************/
/**
 * A decoder that produces a vector of scalar values.
 */
template <typename V, typename C = std::vector<V>>
  requires scalar_or_container_of<C, V>
class Scalar final : Helper<Scalar<V, C>> {
public:
  /// The types of values this decoder can decode.
  using value_type = V;

  /// The range or scalar type.
  using range_type = C;

  /// Constructor.
  Scalar(null_decoder<V> decoder = nullptr)
      : decoder_(std::move(decoder))
  {
  }

  /// Destructor.
  ~Scalar() = default;

  /// Decoding function.
  void decode(const std::shared_ptr<arrow::Array>& src, C& dst)
  {
    using array_type = typename cast_traits<V>::array_type;
    using array_ptr_type = std::shared_ptr<array_type>;

    array_ptr_type casted = std::static_pointer_cast<array_type>(src);

    for (int64_t i : std::views::iota(0, casted->length())) {
      if (casted->IsNull(i)) {
        if (decoder_ != nullptr) {
          std::optional<V> value = std::invoke(decoder_, casted, i);
          if (value.has_value()) this->push(dst, std::move(*value));
        }
      } else {
        this->push(dst, std::move(casted->Value(i)));
      }
    }
  }

private:
  null_decoder<V> decoder_;
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
  List() {};

  /// Destructor.
  ~List() = default;

  /// Decoding function.
  void decode(const std::shared_ptr<arrow::Array>& src, C& dst)
  {
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
