/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/schema/file.h"

namespace MzPeak::Schema {

/******************************************************************************/
void parse_columns(const json::array& input, std::vector<File::Column>& output)
{
  auto get_string = [](const json::object& ob,
                       std::string_view key) -> std::optional<std::string> {
    auto it = ob.find(key);

    if (it != ob.end() && it->value().is_string()) {
      return std::string(it->value().as_string());
    } else {
      return {};
    }
  };

  output.reserve(input.size());

  for (const auto& column : input) {
    if (!column.is_object()) {
      // FIXME: emit a warning
      continue;
    }

    const auto& colobj(column.as_object());

    File::Column fc = {
        .name = get_string(colobj, "name").value_or(""),
        .path = get_string(colobj, "path")
                    .or_else(std::bind(get_string, colobj, "name"))
                    .value_or(""),
        .accession = get_string(colobj, "accession"),
        .unit = get_string(colobj, "unit"),
    };

    output.push_back(fc);
  }
}

/******************************************************************************/
File::File(const std::string& name)
    : file_name_(name)
{
}

/******************************************************************************/
File::File(const json::object& o)
    : file_name_(o.at("name").as_string())
    , data_kind_(data_kind_from_string(o.at("data_kind").as_string()))
    , entity_type_(entity_type_from_string(o.at("entity_type").as_string()))
    , columns_()
{
  auto cs = o.find("column_mapping");

  if (cs != o.end() && cs->value().is_array()) {
    parse_columns(cs->value().as_array(), columns_);
  }
}

/******************************************************************************/
bool File::is_associated_with(const File& other) const
{
  std::string::size_type underscore(file_name_.find("_"));
  if (underscore == std::string::npos) return false;
  if (other.file_name_.size() < underscore) return false;

  if (file_name_.compare(0, underscore, other.file_name_, 0, underscore) != 0) {
    return false;
  }

  return (data_kind_ == DataKind::DataArray &&
          other.data_kind_ == DataKind::Metadata) ||
         (data_kind_ == DataKind::Metadata &&
          other.data_kind_ == DataKind::DataArray);
}

/******************************************************************************/
bool File::operator==(const File& other) const
{
  return file_name_ == other.file_name_;
};
} // namespace MzPeak::Schema
