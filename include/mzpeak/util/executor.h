/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include "mzpeak/schema/struct.h"
#include "mzpeak/util/planner.h"
#include "mzpeak/util/slice.h"

namespace parquet::arrow {
class FileReader;
}

namespace MzPeak::Util {

class Parquet;

/**
 * Query executor.
 */
class Executor final {
public:
  /// Clean up signatures.
  using Slice = MzPeak::Util::Slice;

  /// Which fields should be extracted.
  using Projection = std::vector<Schema::Column>;

  /// Destructor.
  ~Executor();

  /**
   * Execute a query and extract the requested projection.
   */
  std::unique_ptr<Slice> execute(const Planner::Plan&);

private:
  friend class Parquet;

  /// Constructor.
  Executor(std::shared_ptr<parquet::arrow::FileReader>, const Projection&);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Util
