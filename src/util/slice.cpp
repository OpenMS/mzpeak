/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include "mzpeak/util/slice.h"
#include <memory>

namespace MzPeak::Util {

/******************************************************************************/
struct Slice::Impl {
public:
  Impl(const std::vector<Query::destination_t>& fields)
      : fields_(fields)
  {
  }

  /// Get the array key for the given field.
  Struct::index_type key(const Query::destination_t& field) const
  {
    return field.second->absolute_index();
  }

  std::vector<Query::destination_t> fields_;
  std::map<Struct::index_type, std::shared_ptr<Slice::Raw>> arrays_;
};

/******************************************************************************/
Slice::Slice(const std::vector<Query::destination_t>& fields)
    : impl_(std::make_unique<Impl>(fields))
{
}

/******************************************************************************/
Slice::~Slice() = default;

/******************************************************************************/
const std::vector<Query::destination_t> Slice::fields() const
{
  return impl_->fields_;
}

/******************************************************************************/
std::shared_ptr<Slice::Raw> Slice::raw(const Query::destination_t& field) const
{
  auto it = impl_->arrays_.find(impl_->key(field));

  if (it == impl_->arrays_.end()) {
    return nullptr;
  } else {
    return it->second;
  }
}

/******************************************************************************/
void Slice::append(const Query::destination_t& field,
                   std::shared_ptr<arrow::Array> array)
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
