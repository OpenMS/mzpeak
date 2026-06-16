/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include "mzpeak/query.h"

namespace arrow {
class Array;
}

namespace MzPeak::Util {

class Executor;

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
  const std::vector<Query::destination_t> fields() const;

  /**
   * Return the raw array for the given field.
   *
   * NOTE: If you request a field that does not exist in the slice
   * this function will return a nullptr.
   */
  std::shared_ptr<Raw> raw(const Query::destination_t&) const;

private:
  friend class Executor;

  /// Constructor.
  Slice(const std::vector<Query::destination_t>&);

  /// Add an array chunk.
  void append(const Query::destination_t&, std::shared_ptr<arrow::Array>);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Util
