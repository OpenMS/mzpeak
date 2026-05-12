/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <parquet/metadata.h>

#include "mzpeak/util/enumerable_proxy.h"

namespace MzPeak::Util {

/**
 * Helper for accessing row group metatdata.
 */
class RowGroupMetadataProxy final
    : public Util::EnumerableProxy<parquet::RowGroupMetaData> {
public:
  using RowGroupMetadata = std::shared_ptr<parquet::RowGroupMetaData>;
  using ColumnChunkMetadata = Util::EnumerableProxy<parquet::ColumnChunkMetaData>;

  /// Constructor.
  RowGroupMetadataProxy(std::shared_ptr<parquet::FileMetaData> fmd)
      : EnumerableProxy(fmd->num_row_groups(),
                        [&](std::size_t n) { return fmd_->RowGroup(n); }),
        fmd_(fmd) {};

  /// Destructor.
  ~RowGroupMetadataProxy() = default;

  /// Access the column chunk metadata for a row group.
  ColumnChunkMetadata column_chunk(const RowGroupMetadata& rg) const {
    return ColumnChunkMetadata(rg->num_columns(),
                               [rg](std::size_t n) { return rg->ColumnChunk(n); });
  };

private:
  std::shared_ptr<parquet::FileMetaData> fmd_;
};

} // namespace MzPeak::Util
