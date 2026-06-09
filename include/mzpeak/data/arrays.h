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
   * Extract all of the requested arrays from the current table using
   * the given query to limit the resulting data.
   *
   * NOTE: The query should really only contain predicates that match
   * arrays that have a sort ranking of 0.
   */
  std::unique_ptr<Data::array_map_type>
  read_arrays(const Query&, const std::vector<Schema::ArrayIndex::Column>&);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Data
