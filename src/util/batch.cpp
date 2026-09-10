/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <arrow/record_batch.h>
#include <print>

#include "mzpeak/util/batch.h"
#include "mzpeak/util/slice.h"

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
  } else if (batch_->schema()->CanReferenceFieldByName(column.first->name()).ok()) {
    // The column is a struct and we need to reach into it.
    std::shared_ptr<arrow::Array> ary(batch_->GetColumnByName(column.first->name()));

    if (ary && ary->type_id() == arrow::Type::STRUCT) {
      auto sa = std::static_pointer_cast<arrow::StructArray>(ary);
      result = sa->field(column.second->relative_index());
    }
  } else {
    // The batch can also expose the struct fields as columns.
    result = batch_->GetColumnByName(column.second->name());
  }

  if (result == nullptr) {
    throw ParquetError("column not in batch: " + column.first->path(*column.second));
  } else {
    return result;
  }
}

/******************************************************************************/
void Batch::collect(Slice& slice) const
{
  for (const auto& column : slice.fields()) {
    slice.append(column, raw(column));
  }
}

/******************************************************************************/
int64_t Batch::size() const { return batch_->num_rows(); }

} // namespace MzPeak::Util
