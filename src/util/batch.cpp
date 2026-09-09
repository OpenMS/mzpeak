/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <arrow/record_batch.h>

#include "mzpeak/util/batch.h"

namespace MzPeak::Util {

/******************************************************************************/
Batch::Batch(std::shared_ptr<arrow::RecordBatch> batch)
    : batch_(std::move(batch))
{
}

/******************************************************************************/
std::shared_ptr<arrow::Array> Batch::raw(const Schema::Column& column) const
{
  std::shared_ptr<arrow::Array> result = nullptr;

  if (column.first->is_root()) {
    result = batch_->GetColumnByName(column.second->name());
  } else {
    std::shared_ptr<arrow::Array> ary(batch_->GetColumnByName(column.first->name()));

    if (ary && ary->type_id() == arrow::Type::STRUCT) {
      auto sa = std::static_pointer_cast<arrow::StructArray>(ary);
      result = sa->field(column.second->relative_index());
    }
  }

  if (result == nullptr) {
    throw ParquetError("column not in batch: " + column.first->path(*column.second));
  } else {
    return result;
  }
}

/******************************************************************************/
int64_t Batch::size() const { return batch_->num_rows(); }

} // namespace MzPeak::Util
