/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <arrow/array.h>

#include "mzpeak/query.h"
#include "mzpeak/schema/array_index.h"
#include "mzpeak/util/parquet.h"
#include "mzpeak/util/types.h"

namespace MzPeak::Data {

/**
 * Low-level access to a single data table in a Parquet file.
 */
class Arrays {
public:
  /// Constructor.
  Arrays(std::unique_ptr<Util::Parquet> parquet);

  /// Destructor.
  ~Arrays();

  /**
   * Access the ArrayIndex for this data file.
   */
  const std::shared_ptr<Schema::ArrayIndex>& array_index() const;

  /**
   * Get the number of records in this data file.
   */
  std::size_t record_count() const;

  /**
   * Return a query builder for the index column.
   *
   * Useful for index-based queries such as `index().eq(n)`.
   */
  Query::Builder index() const;

  /**
   * Execute a query, projecting the requested dimensions.
   */
  std::unique_ptr<Slice> select(const std::vector<Dimension>&, const Query&);

  /**
   * Low-level interface for accessing a struct field given its name.
   *
   * Useful if you need to manually construct queries.
   */
  std::optional<Util::Column> field(const std::string_view&) const;

  /**
   * Low-level interface for accessing the schema encoded as a map of
     Struct objects.
   */
  const std::shared_ptr<Util::StructMap>& structs() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Data
