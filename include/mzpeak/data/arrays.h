/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <arrow/array.h>

#include "mzpeak/data/encoding.h"
#include "mzpeak/query.h"
#include "mzpeak/schema/array_index.h"
#include "mzpeak/util/parquet.h"
#include "mzpeak/util/struct.h"

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
  const Schema::ArrayIndex& array_index() const;

  /**
   * Get the number of records if this data file.
   */
  std::size_t record_count() const;

  /**
   * Low-level interface for accessing a struct field given its name.
   *
   * Useful if you need to manually construct queries.
   */
  std::optional<Query::destination_t> field(const std::string_view&) const;

  /**
   * Convert array index columns into struct fields.
   *
   * This is a temporary function while we migrate to a new query
     system.
   */
  std::vector<Util::Struct::Field>
  columns_to_fields(const std::vector<Schema::ArrayIndex::Column>&) const;

  /**
   * Extract all of the requested arrays from the current table using
   * the given query to limit the resulting data.
   *
   * NOTE: The query should really only contain predicates that match
   * arrays that have a sort ranking of 0.
   */
  std::unique_ptr<Data::array_map_type>
  read_arrays(const Query&, const std::vector<Util::Struct::Field>&);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Data
