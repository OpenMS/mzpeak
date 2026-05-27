/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <boost/json.hpp>
#include <string>

#include "mzpeak/schema/data_kind.h"
#include "mzpeak/schema/entity_type.h"

namespace MzPeak::Schema {
namespace json = boost::json;

struct File {

  /// Constructor from a file name.
  explicit File(const std::string& name)
      : file_name(name)
  {
  }

  /// Conversion from JSON.
  explicit File(const json::object&);

  /// Return `true` if this file is associated with the given file.
  /// For example, if this file is a DataArray and the other file is a
  /// Metadata file with a similar name.
  bool is_associated_with(const File&) const;

  /// Equality operator.
  bool operator==(const File&) const = default;

  /// The name of this file.
  std::string file_name;

  /// This file's data kind.
  DataKind data_kind = DataKind::Other;

  /// This file's entity type.
  EntityType entity_type = EntityType::Other;
};

} // namespace MzPeak::Schema
