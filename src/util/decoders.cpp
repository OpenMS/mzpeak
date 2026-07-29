/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/util/decoders.h"

namespace MzPeak::Util::Decoders {

/******************************************************************************/
bool is_list_array(const std::shared_ptr<arrow::Array>& ary)
{
  auto t = ary->type_id();

  return t == arrow::Type::LIST || t == arrow::Type::FIXED_SIZE_LIST ||
         t == arrow::Type::LARGE_LIST || t == arrow::Type::LIST_VIEW ||
         t == arrow::Type::LARGE_LIST_VIEW;
}

} // namespace MzPeak::Util::Decoders
