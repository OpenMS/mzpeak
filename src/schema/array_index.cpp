/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <ranges>

#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/buffer_format.h"
#include "mzpeak/schema/entity_type.h"

namespace MzPeak::Schema {

/******************************************************************************/
ArrayIndex::Column make_index(EntityType entity_type, const std::string& prefix)
{
  ArrayIndex::Column index;

  index.array_name = entity_type_to_string(entity_type) + "_index";
  index.buffer_format = BufferFormat::Point;
  index.context = entity_type;
  index.path = prefix + "." + index.array_name;
  index.data_type = PSI::DataType::Int64;
  index.array_type = PSI::ArrayType::NonStandard;
  index.unit = "MS:1000774";
  index.sorting_rank = 0;
  index.buffer_priority = false;

  return index;
}

/******************************************************************************/
ArrayIndex::ArrayIndex(EntityType entity_type, const json::object& obj)
    : entity_type_(entity_type)
    , prefix_(obj.at("prefix").as_string())
{
  auto entries = obj.find("entries");

  if (entries != obj.end() && entries->value().is_array()) {
    auto entries_ary(entries->value().as_array());
    columns_.reserve(entries_ary.size() + 1);

    // The first column entry is actually the index itself.
    columns_.push_back(make_index(entity_type, prefix_));

    for (const auto& entry : entries_ary) {
      if (entry.is_object()) {
        const auto& eo(entry.as_object());
        Column column;
        column.array_name = eo.at("array_name").as_string();
        column.buffer_format =
            buffer_format_from_string(eo.at("buffer_format").as_string());
        column.context = entity_type_from_string(eo.at("context").as_string());
        column.path = eo.at("path").as_string();
        column.data_type =
            PSI::data_type_from_string(eo.at("data_type").as_string());
        column.array_type =
            PSI::array_type_from_string(eo.at("array_type").as_string());
        column.unit = eo.at("unit").as_string();

        if (auto bp = eo.find("buffer_priority");
            bp != eo.end() && bp->value().is_string()) {
          column.buffer_priority = bp->value().as_string() == "primary";
        }

        if (auto sr = eo.find("sorting_rank");
            sr != eo.end() && sr->value().is_number()) {
          if (sr->value().is_int64()) {
            column.sorting_rank = sr->value().as_int64();
          } else {
            column.sorting_rank = sr->value().as_uint64();
          }
        }

        if (auto dpi = eo.find("data_processing_id");
            dpi != eo.end() && dpi->value().is_string()) {
          column.data_processing_id = dpi->value().as_string();
        }

        if (auto tr = eo.find("transform");
            tr != eo.end() && tr->value().is_string()) {
          column.transform = tr->value().as_string();
        }

        columns_.push_back(std::move(column));
      }
    }
  }
}

/******************************************************************************/
EntityType ArrayIndex::entity_type() const { return entity_type_; }

/******************************************************************************/
const std::string& ArrayIndex::prefix() const { return prefix_; }

/******************************************************************************/
const std::vector<ArrayIndex::Column>& ArrayIndex::columns() const
{
  return columns_;
}

/******************************************************************************/
std::vector<ArrayIndex::Column> ArrayIndex::columns(PSI::ArrayType type) const
{
  return columns_ |
         std::views::filter([type](const auto& c) { return c.array_type == type; }) |
         std::ranges::to<std::vector>();
}

/******************************************************************************/
void ArrayIndex::num_entities(const std::optional<std::size_t>& ne)
{
  num_entities_ = ne;
}

/******************************************************************************/
std::optional<std::size_t> ArrayIndex::num_entities() const { return num_entities_; }

/******************************************************************************/
std::optional<int> ArrayIndex::column_index(const Column& column) const
{
  auto it = column_map_.find(column.path);

  if (it == column_map_.end()) {
    return {};
  } else {
    return it->second;
  }
}

/******************************************************************************/
void ArrayIndex::column_map(ColumnMap column_map)
{
  column_map_ = std::move(column_map);
}

} // namespace MzPeak::Schema
