/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <functional>
#include <parquet/metadata.h>
#include <parquet/statistics.h>

#include "mzpeak/io/file.h"
#include "mzpeak/schema/file.h"
#include "mzpeak/schema/group.h"
#include "mzpeak/util/batch.h"
#include "mzpeak/util/compat.h" // IWYU pragma: keep
#include "mzpeak/util/executor.h"
#include "mzpeak/util/query.h"

namespace MzPeak::Util {

/**
 * Low-level wrapper around Parquet files.
 */
class Parquet final {
public:
  using file_metadata_t = std::shared_ptr<parquet::FileMetaData>;

  /// Constructor.
  Parquet(std::unique_ptr<MzPeak::IO::File>, Schema::File);

  /// Destructor.
  ~Parquet();

  /**
   * Return the file information from the MzPeak index.
   */
  const Schema::File& index_file() const;

  /**
   * Return the schema encoded as a map of Group objects.
   */
  const std::shared_ptr<Schema::GroupMap>& groups() const;

  /**
   * Return a Group and Field matching the given names.
   */
  std::optional<Schema::Column> field(std::string_view, std::string_view) const;

  /**
   * Access the file metadata.
   */
  file_metadata_t file_metadata() const;

  /**
   * Fetch a string value from the metadata key-value store.
   */
  std::optional<std::string> kv_string(const file_metadata_t&,
                                       std::string_view) const;

  /**
   * Fetch a std::size_t value from the metadata key-value store.
   */
  std::optional<std::size_t> kv_size_t(const file_metadata_t&,
                                       std::string_view) const;

  /**
   * Directly access the FileReader.  This reference is only valid
   * while this Parquet object exists.
   */
  parquet::arrow::FileReader& reader() const;

  /**
   * A function that accepts one batch at a time.  It should return
   * `true` to indicate it wants more batches or `false` to stop
   * feeding batches.
   */
  using Reader = std::move_only_function<bool(const Batch&)>;

  /**
   * Read all rows from a table.
   */
  void read(Reader, const Projection&) const;

  /**
   * Return a planner for the given query.
   */
  Planner planner(const Query&);

  /**
   * Return an executor that will capture the requested fields.
   */
  Executor executor(const Projection&);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Util
