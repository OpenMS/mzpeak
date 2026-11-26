/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <generator>
#include <memory>

#include "mzpeak/util/compat.h" // IWYU pragma: keep

namespace parquet::arrow {
class FileReader;
}

namespace MzPeak::Util {

class Batch;
class Parquet;
class Plan;
class Projection;
class Query;
class Slice;

/**
 * Query executor.
 */
class Executor final {
public:
  /// Destructor.
  ~Executor();

  /**
   * Read batches returned from a Query.
   */
  std::generator<const Batch&> read(const Query&, const Plan&) const;

  /**
   * Execute a query and extract the requested projection.
   */
  std::unique_ptr<Slice> execute(const Query&, const Plan&);

private:
  friend class Parquet;

  /// Constructor.
  Executor(std::shared_ptr<parquet::arrow::FileReader>, const Projection&);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Util
