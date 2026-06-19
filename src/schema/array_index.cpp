/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <algorithm>
#include <iterator>
#include <ranges>

#include "mzpeak/schema/array_index.h"
#include "mzpeak/schema/buffer_format.h"
#include "mzpeak/schema/entity_type.h"

namespace MzPeak::Schema {

/******************************************************************************/
ArrayIndex::ArrayIndex(EntityType entity_type, const json::object& obj)
    : entity_type_(entity_type)
    , prefix_(obj.at("prefix").as_string())
{
  auto entries = obj.find("entries");

  if (entries != obj.end() && entries->value().is_array()) {
    auto entries_ary(entries->value().as_array());
    columns_.reserve(entries_ary.size() + 1);

    for (const auto& entry : entries_ary) {
      if (entry.is_object()) {
        const auto& eo(entry.as_object());
        Column column;
        column.array_name = eo.at("array_name").as_string();
        column.buffer_format =
            buffer_format_from_string(eo.at("buffer_format").as_string());
        column.context = entity_type_from_string(eo.at("context").as_string());
        column.path = eo.at("path").as_string();
        column.name = column.path.substr(prefix_.size() + 1);
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

  std::ranges::sort(columns_, {}, &Column::array_name);
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
std::vector<ArrayIndex::Column> ArrayIndex::columns(const Data::Dimension& d) const
{
  return columns_ | std::views::filter([d](const auto& c) {
           return c.array_type == d.array_type && c.data_type == d.data_type;
         }) |
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
std::vector<Data::Dimension> ArrayIndex::dimensions() const
{

  std::vector<Column> cols;
  cols.reserve(columns_.size()); // Should be small

  std::ranges::unique_copy(columns_.begin(), columns_.end(),
                           std::back_inserter(cols), [](auto& a, auto& b) {
                             return a.array_name == b.array_name &&
                                    a.data_type == b.data_type &&
                                    a.array_type == b.array_type;
                           });

  std::vector<Data::Dimension> res;
  res.reserve(cols.size());

  for (auto& col : cols) {
    res.push_back({col.array_name, col.data_type, col.array_type});
  }

  return res;
}

/******************************************************************************/
std::optional<Util::Column> ArrayIndex::entry_column(const Util::StructMap& map,
                                                     const Column& col)
{
  auto it = map.find(prefix_);
  if (it == map.end()) return {};

  auto field = it->second->field(col.name);
  if (!field.has_value()) return {};

  return std::make_pair(it->second, field.value());
}

} // namespace MzPeak::Schema
