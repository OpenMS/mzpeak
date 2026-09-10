/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <memory>

#include "mzpeak/schema/group.h"
#include "mzpeak/util/decoders.h"
#include "mzpeak/util/types.h"

namespace arrow {
class RecordBatch;
class Array;
} // namespace arrow

namespace MzPeak::Util {

class Parquet;
class Slice;

/**
 * Simple interface for Arrow record batches.
 */
class Batch final {
public:
  /**
   * Access a single column from the batch.
   */
  std::shared_ptr<arrow::Array> raw(const Schema::Column&) const;

  /**
   * Return a single value for the given row and column.
   */
  template <supported_type T, typename Container = std::optional<T>>
  Container scalar(int64_t, const Schema::Column&) const;

  /**
   * Return a single value for the given row and column.
   */
  template <supported_type T, typename Container = std::optional<T>>
  Container scalar(int64_t, const std::optional<Schema::Column>&) const;

  /**
   * Append all batched columns into the given slice.
   */
  void collect(Slice&) const;

  /**
   * Return the number of rows.
   */
  int64_t size() const;

protected:
  friend class MzPeak::Util::Parquet;

  /// Constructor.
  Batch(std::shared_ptr<arrow::RecordBatch>);

private:
  std::shared_ptr<arrow::RecordBatch> batch_;
};

/******************************************************************************/
template <supported_type T, typename Container>
Container Batch::scalar(int64_t row, const Schema::Column& column) const
{
  if constexpr (std::is_same_v<Container, std::vector<T>>) {
    return Decoders::lift_list_array(
        raw(column), [&]<typename L>(const std::shared_ptr<L>& list) {
          Container res;

          if (list->IsValid(row)) {
            Decoders::Scalar<T, Container> decoder;
            decoder.decode(list->value_slice(row), res);
            return res;
          } else {
            return res;
          }
        });
  } else {
    using A = type_traits<enum_type_v<T>>::array_type;
    auto array = std::static_pointer_cast<A>(raw(column));

    if (array->IsValid(row)) {
      return Decoders::unsafe_array_value<enum_type_v<T>>(array, row);
    } else {
      return std::nullopt;
    }
  }
}

/******************************************************************************/
template <supported_type T, typename Container>
Container Batch::scalar(int64_t row,
                        const std::optional<Schema::Column>& column) const
{
  if (column.has_value()) {
    return scalar<T, Container>(row, column.value());
  } else {
    return {};
  }
}

} // namespace MzPeak::Util
