/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <parquet/api/reader.h>

#include "mzpeak/index/entity_type.h"

namespace MzPeak {

/**
 * Low-level wrapper around Parquet files.
 */
class Parquet final {
public:
  /// Constructor.
  Parquet(std::unique_ptr<parquet::ParquetFileReader>);

  /// Destructor.
  ~Parquet();

  /**
   * Map over the row group metadata and return the results.
   */
  template <typename T>
  std::vector<T>
      map_row_group_metadata(std::function<T(const parquet::RowGroupMetaData&)>);

  /**
   * Fetch an entry from the file metadata store.
   */
  std::optional<std::string> metadata_kv(const std::string& key) const;

  /**
   * Return the number of mzpeak entities in this file.
   */
  std::size_t num_entities(Index::EntityType);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak
