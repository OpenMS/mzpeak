/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include "mzpeak/exception.h"
#include "mzpeak/query.h"
#include "mzpeak/util/parquet_types.h"
#include <parquet/statistics.h>

namespace MzPeak {

/******************************************************************************/
Query::Query(Util::Parquet::file_metadata_t file_metadata)
    : file_metadata_(file_metadata) {}

/******************************************************************************/
const Query::Location& Query::find(std::function<Result(Cursor&)> func) {
  location_.row_group_indices.clear();

  Cursor cursor(file_metadata_);
  Result result = std::make_pair<>(Action::Skip, 1);

  auto record_location = [&]() {
    location_.row_group_indices.push_back(cursor.row_group_idx_);
  };

  for (bool go = true; go && cursor.valid(); cursor.next(result.second)) {
    result = std::invoke(func, cursor);

    switch (result.first) {
    case Action::Skip:
      break;
    case Action::Stop:
      go = false;
      break;
    case Action::Match:
      record_location();
      break;
    case Action::MatchStop:
      record_location();
      go = false;
      break;
    }
  }

  return location_;
}

/******************************************************************************/
Query::Cursor::Cursor(Util::Parquet::file_metadata_t file_metadata)
    : file_metadata_(file_metadata),
      num_row_groups_(file_metadata_->num_row_groups()) {}

/******************************************************************************/
bool Query::Cursor::valid() const { return row_group_idx_ < num_row_groups_; }

/******************************************************************************/
void Query::Cursor::next(std::size_t n) {
  row_group_idx_ += n;
  col_chunk_idx_ = 0;
  row_group_meta_.reset(nullptr);
  col_chunk_.reset(nullptr);
  col_stats_.reset();
}

/******************************************************************************/
bool Query::Cursor::load(const std::string& path) {
  if (!valid()) return false;
  if (!row_group_meta_) row_group_meta_ = file_metadata_->RowGroup(row_group_idx_);

  int column_index = row_group_meta_->schema()->ColumnIndex(path);
  if (column_index < 0) return false; /// FIXME: probably throw here.

  if (!col_chunk_ || col_chunk_idx_ != static_cast<std::size_t>(column_index)) {
    if (col_stats_) col_stats_.reset();
    col_chunk_idx_ = column_index;
    col_chunk_ = row_group_meta_->ColumnChunk(col_chunk_idx_);
  }

  if (!col_stats_) {
    if (!col_chunk_->is_stats_set()) return false;
    if (col_stats_ = col_chunk_->statistics(); !col_stats_) return false;
  }

  return col_stats_->HasMinMax();
}

} // namespace MzPeak
