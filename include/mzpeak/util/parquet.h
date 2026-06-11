/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <parquet/metadata.h>
#include <parquet/statistics.h>

#include "mzpeak/file.h"
#include "mzpeak/query.h"
#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/file.h"
#include "mzpeak/util/struct.h"

namespace MzPeak::Util {

/**
 * Low-level wrapper around Parquet files.
 */
class Parquet final {
public:
  using file_metadata_t = std::shared_ptr<parquet::FileMetaData>;

  using struct_map_t = std::map<std::string, std::shared_ptr<Struct>>;

  /// Constructor.
  Parquet(std::unique_ptr<MzPeak::File>, Schema::File);

  /// Destructor.
  ~Parquet();

  /**
   * Return the file information from the MzPeak index.
   */
  const Schema::File& index_file() const;

  /**
   * Return the schema encoded as a map of Struct objects.
   */
  const struct_map_t& structs() const;

  /**
   * Return a Struct and Field matching the given names.
   */
  std::optional<Query::destination_t> field(const std::string_view&,
                                            const std::string_view&) const;

  /**
   * Access the file metadata.
   */
  file_metadata_t file_metadata() const;

  /**
   * Return the raw array index JSON.
   */
  std::string array_index_json() const;

  /**
   * Parse and return the ArrayIndex.
   */
  Schema::ArrayIndex array_index() const;

  /**
   * Directly access the FileReader.  This reference is only valid
   * while this Parquet object exists.
   */
  parquet::arrow::FileReader& reader() const;

  /**
   * Execute a query and return the row groups that matched.
   */
  std::vector<int> find_row_groups(const Query&);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Util
