/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <arrow/array.h>

#include "mzpeak/schema/group.h"
#include "mzpeak/util/compat.h" // IWYU pragma: keep
#include "mzpeak/util/decoders.h"

namespace MzPeak::Util {

// Forward declarations.
class Executor;

/******************************************************************************/
using Column = MzPeak::Schema::Column;

/******************************************************************************/
/**
 * Represents a subset of a Parquet file resulting from executing a
 * query.
 */
class Slice final {
public:
  /// Raw, chunked arrays from Parquet.
  using Raw = std::vector<std::shared_ptr<arrow::Array>>;

  /// Destructor.
  ~Slice();

  /**
   * Return a list of fields that can be extracted from this slice.
   */
  const std::vector<Column>& fields() const;

  /**
   * Return true if the given column is in the slice.
   */
  bool has_column(const Column&) const;

  /**
   * Return the raw array for the given field.
   *
   * NOTE: If you request a field that does not exist in the slice
   * this function will return a nullptr.
   */
  std::shared_ptr<Raw> raw(const Column&) const;

  /**
   * Decode the first non-null value.
   *
   * Template Parameters:
   *
   *   - T: The Decoder class to use (see MzPeak::Util::Decoders)
   *
   *   - R: The destination object to update with the decoded value
   */
  template <typename T, typename R = std::optional<typename T::value_type>>
  void singleton(const Column&, R&, T&& = {}) const;

  /**
   * Exact and decode an array.
   *
   * Use one of the decoders defined in `decoders.h`, or write your own.
   *
   * Template Parameters:
   *
   *   - T: The Decoder class to use (see MzPeak::Util::Decoders)
   *
   *   - V: The destination object to fill with decoded values
   */
  template <typename T, typename V = std::vector<typename T::value_type>>
    requires Decoders::from_arrow_array<T, V>
  void array(const Column&, V&, T&& = {}) const;

  /****************************************************************************/
  template <typename T, typename V = std::vector<typename T::value_type>>
    requires Decoders::from_arrow_array<T, V>
  void array(const Column&, V&, T&) const;

private:
  friend class MzPeak::Util::Executor;

  /// Constructor.
  Slice(const std::vector<Column>&);

  /// Add an array chunk.
  void append(const Column&, std::shared_ptr<arrow::Array>);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

/******************************************************************************/
template <typename T, typename R>
void Slice::singleton(const Column& field, R& dst, T&& t) const
{
  std::shared_ptr<Raw> chunks = raw(field);

  if (chunks != nullptr) {
    for (const auto& chunk : *chunks) {
      for (int64_t i : std::views::iota(0, chunk->length())) {
        if (!chunk->IsNull(i)) {
          std::shared_ptr<arrow::Array> sliced = chunk->Slice(i, 1);

          if constexpr (std::is_same_v<R, typename T::value_type>) {
            t.decode(sliced, dst);
            return;
          } else {
            typename T::value_type val{};
            t.decode(sliced, val);
            dst = {std::move(val)};
            return;
          }
        }
      }
    }
  }

  dst = {};
}

/******************************************************************************/
template <typename T, typename V>
  requires Decoders::from_arrow_array<T, V>
void Slice::array(const Column& field, V& v, T&& t) const
{
  array(field, v, t);
}

/******************************************************************************/
template <typename T, typename V>
  requires Decoders::from_arrow_array<T, V>
void Slice::array(const Column& field, V& v, T& t) const
{
  std::shared_ptr<Raw> chunks = raw(field);
  if (chunks == nullptr) return;

  if constexpr (requires { v.reserve(std::size_t{}); }) {
    std::size_t size{};

    for (const auto& chunk : *chunks) {
      size += Decoders::guess_array_length(field, chunk);
    }

    v.reserve(v.size() + size);
  }

  for (const auto& chunk : *chunks) {
    t.decode(chunk, v);
  }
}

} // namespace MzPeak::Util
