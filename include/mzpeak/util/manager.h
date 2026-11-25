/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "mzpeak/io/archive.h"
#include "mzpeak/schema/file.h"

namespace MzPeak::Util {

class Parquet;

/**
 * File manager for parquet files.
 */
class Manager final {
public:
  /// Constructor.
  Manager(std::unique_ptr<MzPeak::IO::Archive>);

  /// Destructor.
  ~Manager();

  /**
   * Return a vector of files that are located in the mzPeak archive.
   */
  const std::vector<Schema::File>& files() const;

  /**
   * Find a file given its name.
   */
  std::vector<Schema::File>::const_iterator find_file(std::string_view) const;

  /**
   * Find a file given its `EntityType` and `DataKind`.
   */
  std::vector<Schema::File>::const_iterator find_file(Schema::EntityType::Type,
                                                      Schema::DataKind::Type) const;

  /**
   * Open a Parquet file from the mzPeak archive.
   */
  std::shared_ptr<Parquet> parquet(const Schema::File&);

  /**
   * Return the maximum size allowed for metadata caching.
   */
  std::size_t metadata_cache_size() const;

  /**
   * Reset and clear the internal cache.
   */
  void clear();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Util
