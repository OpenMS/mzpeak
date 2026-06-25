/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <memory>

#include "mzpeak/schema/struct.h"
#include "mzpeak/util/parquet.h"
#include "mzpeak/util/projection.h"

namespace MzPeak::Metadata {

/**
 * Low-level access to metadata files in a mzpeak file.
 */
class Table final {
public:
  /// Constructor.
  Table(std::unique_ptr<Util::Parquet>);

  /// Destructor.
  ~Table();

  /**
   * Return a struct with the given name.  If the struct does not
   * exist in the schema return `nullptr`.
   */
  std::shared_ptr<Schema::Struct> group(const std::string_view&) const;

  /**
   * Read all rows from the given struct where the index column
   * matches the given value.
   *
   * Returns a slice with the given column projection.
   */
  std::unique_ptr<Util::Slice> indexed(uint64_t,
                                       const std::shared_ptr<Schema::Struct>&,
                                       const Util::Projection&) const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Metadata
