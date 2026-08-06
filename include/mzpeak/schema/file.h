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

/**
 * A description of a file in the mzPeak archive.
 */
class File final {
public:
  struct Column {
    std::string name;
    std::string path;
    std::optional<std::string> accession;
    std::optional<std::string> unit;
  };

  /// Constructor from a file name.
  explicit File(const std::string& name);

  /// Conversion from JSON.
  explicit File(const json::object&);

  /// Return `true` if this file is associated with the given file.
  /// For example, if this file is a DataArray and the other file is a
  /// Metadata file with a similar name.
  bool is_associated_with(const File&) const;

  /// Equality operator.
  bool operator==(const File&) const;

  /// The name of this file.
  const std::string& file_name() const { return file_name_; }

  /// This file's data kind.
  DataKind data_kind() const { return data_kind_; }

  /// This file's entity type.
  EntityType entity_type() const { return entity_type_; }

  /// Column definitions for this file.
  const std::vector<Column>& columns() const { return columns_; }

private:
  std::string file_name_;
  DataKind data_kind_ = DataKind::Other;
  EntityType entity_type_ = EntityType::Other;
  std::vector<Column> columns_;
};

} // namespace MzPeak::Schema
