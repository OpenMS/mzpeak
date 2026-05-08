/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/index/file.h"

namespace MzPeak::Index {

/******************************************************************************/
File::File(const json::object& o)
    : file_name(o.at("name").as_string()),
      data_kind(data_kind_from_string(o.at("data_kind").as_string())),
      entity_type(entity_type_from_string(o.at("entity_type").as_string())) {}

/******************************************************************************/
bool File::is_associated_with(const File& other) const {
  std::string::size_type underscore(file_name.find("_"));
  if (underscore == std::string::npos) return false;
  if (other.file_name.size() < underscore) return false;

  if (file_name.compare(0, underscore, other.file_name, 0, underscore) != 0) {
    return false;
  }

  return (data_kind == DataKind::DataArray &&
          other.data_kind == DataKind::Metadata) ||
         (data_kind == DataKind::Metadata && other.data_kind == DataKind::DataArray);
}

} // namespace MzPeak::Index
