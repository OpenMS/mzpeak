/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <parquet/metadata.h>

#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/entity_type.h"
#include "mzpeak/util/arrow.h"
#include "mzpeak/util/row_group_metadata_proxy.h"

namespace MzPeak::Util {

/**
 * Low-level wrapper around Parquet files.
 */
class Parquet final {
public:
  using file_metadata_t = std::shared_ptr<parquet::FileMetaData>;

  /// Constructor.
  Parquet(const Arrow&);

  /// Destructor.
  ~Parquet();

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
  Schema::ArrayIndex array_index(Schema::EntityType) const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Util
