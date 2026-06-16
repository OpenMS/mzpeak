/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <cstdint>
#include <memory>

#include "mzpeak/query.h"

namespace parquet::arrow {
class FileReader;
}

namespace MzPeak::Util {

class Parquet;

/**
 * Create a plan for the most efficient way to execute a query against
 * a Parquet file.
 */
class Planner final {
public:
  /// How to limit table reads.
  struct Range {
    int32_t row_group; /// The row group to request.
    int64_t offset;    /// The first row to read.
    int64_t length;    /// The number of rows to read.
  };

  /// Information about how to carry out a query.
  struct Plan {
    Query query;
    std::vector<Range> ranges;
  };

  /// Destructor.
  ~Planner();

  /**
   * Construct a query plan.
   */
  const Plan& plan() const;

private:
  friend class Parquet;

  /// Constructor.
  Planner(parquet::arrow::FileReader&, const Query&);

  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Util
