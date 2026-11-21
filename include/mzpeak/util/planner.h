/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <memory>

#include "mzpeak/util/query.h"

namespace parquet::arrow {
class FileReader;
}

namespace MzPeak::Util {

class Parquet;
class Index;

/**
 * Create a plan for the most efficient way to execute a query against
 * a Parquet file.
 */
class Planner final {
public:
  /**
   * Construct a query plan.
   */
  Plan plan(const Query&) const;

  /// Destructor.
  ~Planner();

  /// Read-only access to the index.  Useful for debugging.
  const Index& index() const;

private:
  friend class Parquet;

  /// Constructor.
  Planner(parquet::arrow::FileReader&, std::shared_ptr<Schema::GroupMap> groups_);

  class Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Util
