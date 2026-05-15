/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <parquet/metadata.h>

#include "mzpeak/file.h"
#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/file.h"
#include "mzpeak/util/row_group_metadata_proxy.h"

namespace MzPeak::Util {

/**
 * Low-level wrapper around Parquet files.
 */
class Parquet final {
public:
  using file_metadata_t = std::shared_ptr<parquet::FileMetaData>;

  /// Constructor.
  Parquet(std::unique_ptr<File::Readable>, Schema::File);

  /// Destructor.
  ~Parquet();

  /**
   * Return the file information from the MzPeak index.
   */
  const Schema::File& index_file() const;

  /**
   * Access the file metadata.
   */
  file_metadata_t file_metadata() const;

  /**
   *
   */
  Util::RowGroupMetadataProxy rg_metadata() const;

  /**
   * Parse and return the ArrayIndex.
   */
  Schema::ArrayIndex array_index() const;

  /**
   * Directly access the FileReader.  This reference is only valid
   * while this Parquet object exists.
   */
  parquet::arrow::FileReader& reader() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Util
