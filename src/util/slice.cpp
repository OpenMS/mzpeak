/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include "mzpeak/util/slice.h"

#include <memory>

namespace MzPeak::Util {

using Struct = MzPeak::Schema::Struct;

/******************************************************************************/
struct Slice::Impl {
public:
  Impl(const std::vector<Column>& fields)
      : fields_(fields)
  {
  }

  /// Get the array key for the given field.
  Struct::index_type key(const Column& field) const
  {
    return field.second->absolute_index();
  }

  std::vector<Column> fields_;
  std::map<Struct::index_type, std::shared_ptr<Slice::Raw>> arrays_;
};

/******************************************************************************/
Slice::Slice(const std::vector<Column>& fields)
    : impl_(std::make_unique<Impl>(fields))
{
}

/******************************************************************************/
Slice::~Slice() = default;

/******************************************************************************/
const std::vector<Column>& Slice::fields() const { return impl_->fields_; }

/******************************************************************************/
bool Slice::has_column(const Column& column) const
{
  return impl_->arrays_.contains(impl_->key(column));
}

/******************************************************************************/
std::shared_ptr<Slice::Raw> Slice::raw(const Column& field) const
{
  auto it = impl_->arrays_.find(impl_->key(field));

  if (it == impl_->arrays_.end()) {
    return nullptr;
  } else {
    return it->second;
  }
}

/******************************************************************************/
void Slice::append(const Column& field, std::shared_ptr<arrow::Array> array)
{
  auto it = impl_->arrays_.find(impl_->key(field));

  if (it == impl_->arrays_.end()) {
    std::shared_ptr<Raw> v = std::make_shared<Raw>();
    v->push_back(array);
    impl_->arrays_[impl_->key(field)] = v;
  } else {
    it->second->push_back(array);
  }
}

} // namespace MzPeak::Util
