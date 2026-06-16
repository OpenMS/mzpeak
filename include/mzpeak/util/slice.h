/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <arrow/array.h>
#include <functional>
#include <ranges>

#include "mzpeak/query.h"
#include "mzpeak/util/compat.h" // IWYU pragma: keep

/******************************************************************************/
/**
 * A type that can decode arrow arrays, inserting the values into a vector.
 */
template <typename T>
concept from_arrow_array = requires(T t,
                                    const std::shared_ptr<arrow::Array>& a,
                                    std::vector<typename T::value_type>& v) {
  { t.decode(a, v) } -> std::same_as<void>;
};

namespace MzPeak::Util {

// Forward declarations.
class Executor;

/******************************************************************************/
/**
 * Type traits for casting.
 */
template <typename T> struct cast_traits;

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
 * Represents a subset of a Parquet file resulting from executing a
 * query.
 */
class Slice final {
public:
  /// Raw, chunked arrays from Parquet.
  using Raw = std::vector<std::shared_ptr<arrow::Array>>;

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

  /// Destructor.
  ~Slice();

  /**
   * Return a list of fields that can be extracted from this slice.
   */
  const std::vector<Query::destination_t> fields() const;

  /**
   * Return the raw array for the given field.
   *
   * NOTE: If you request a field that does not exist in the slice
   * this function will return a nullptr.
   */
  std::shared_ptr<Raw> raw(const Query::destination_t&) const;

  /**
   * Exact and decode an array.
   *
   * Use one of the decoders defined below, or write your own.
   */
  template <from_arrow_array T>
  std::vector<typename T::value_type> array(const Query::destination_t&,
                                            T = {}) const;

  /**
   * A decoder that produces a vector of scalar values.
   */
  template <typename T> class DecodeScalar final {
  public:
    /// The types of values this decoder can decode.
    using value_type = T;

    /// Constructor.
    DecodeScalar(null_decoder<T> decoder = nullptr)
        : decoder_(std::move(decoder))
    {
    }

    /// Destructor.
    ~DecodeScalar() = default;

    /// Decoding function.
    void decode(const std::shared_ptr<arrow::Array>&, std::vector<value_type>&);

  private:
    null_decoder<T> decoder_;
  };

  /**
   * A decoder where array elements are lists.
   *
   * NULL `ListArray` elements, and NULL elements inside the
   * `ListArray` are skipped.
   */
  template <typename T> class DecodeList final {
  public:
    /// Decodes vectors of type T.
    using value_type = std::vector<T>;

    /// Constructor.
    DecodeList();

    /// Destructor.
    ~DecodeList() = default;

    /// Decoding function.
    void decode(const std::shared_ptr<arrow::Array>&, std::vector<value_type>&);
  };

private:
  friend class Executor;

  /// Constructor.
  Slice(const std::vector<Query::destination_t>&);

  /// Add an array chunk.
  void append(const Query::destination_t&, std::shared_ptr<arrow::Array>);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/******************************************************************************/
template <from_arrow_array T>
std::vector<typename T::value_type> Slice::array(const Query::destination_t& field,
                                                 T t) const
{
  std::shared_ptr<Raw> chunks = raw(field);
  if (chunks == nullptr) return {};

  std::size_t size{};

  for (const auto& chunk : *chunks) {
    size += chunk->length();
  }

  std::vector<typename T::value_type> res;
  res.reserve(size);

  for (const auto& chunk : *chunks) {
    t.decode(chunk, res);
  }

  return res;
}

/******************************************************************************/
template <typename T>
void Slice::DecodeScalar<T>::decode(const std::shared_ptr<arrow::Array>& src,
                                    std::vector<value_type>& dst)
{
  using array_type = typename cast_traits<T>::array_type;
  using array_ptr_type = std::shared_ptr<array_type>;

  array_ptr_type casted = std::static_pointer_cast<array_type>(src);

  for (int64_t i : std::views::iota(0, casted->length())) {
    if (casted->IsNull(i)) {
      if (decoder_ != nullptr) {
        std::optional<T> value = std::invoke(decoder_, casted, i);
        if (value.has_value()) dst.push_back(*value);
      }
    } else {
      dst.push_back(casted->Value(i));
    }
  }
}

/******************************************************************************/
template <typename T>
void Slice::DecodeList<T>::decode(const std::shared_ptr<arrow::Array>& src,
                                  std::vector<value_type>& dst)
{
  std::shared_ptr<arrow::ListArray> casted =
      std::static_pointer_cast<arrow::ListArray>(src);

  for (int64_t i : std::views::iota(0, casted->length())) {
    if (!casted->IsNull(i)) {
      std::shared_ptr<arrow::Array> values(casted->value_slice(i));
      std::vector<T> res;
      res.reserve(values->length());
      DecodeScalar<T>().decode(values, res);
      dst.push_back(res);
    }
  }
}

} // namespace MzPeak::Util
