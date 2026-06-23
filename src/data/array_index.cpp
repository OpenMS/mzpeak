/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/data/array_index.h"

#include <algorithm>
#include <iterator>
#include <ranges>

#include "mzpeak/schema/buffer_format.h"
#include "mzpeak/schema/entity_type.h"

namespace MzPeak::Data {

/******************************************************************************/
ArrayIndex::ArrayIndex(EntityType entity_type, const json::object& obj)
    : entity_type_(entity_type)
    , prefix_(obj.at("prefix").as_string())
{
  auto entries = obj.find("entries");

  if (entries != obj.end() && entries->value().is_array()) {
    auto entries_ary(entries->value().as_array());
    entries_.reserve(entries_ary.size() + 1);

    for (const auto& entry : entries_ary) {
      if (entry.is_object()) {
        const auto& eo(entry.as_object());
        Entry entry;
        entry.array_name = eo.at("array_name").as_string();
        entry.buffer_format =
            buffer_format_from_string(eo.at("buffer_format").as_string());
        entry.context = entity_type_from_string(eo.at("context").as_string());
        entry.path = eo.at("path").as_string();
        entry.name = entry.path.substr(prefix_.size() + 1);
        entry.data_type = PSI::data_type_from_string(eo.at("data_type").as_string());
        entry.array_type =
            PSI::array_type_from_string(eo.at("array_type").as_string());
        entry.unit = eo.at("unit").as_string();

        if (auto bp = eo.find("buffer_priority");
            bp != eo.end() && bp->value().is_string()) {
          entry.buffer_priority = bp->value().as_string() == "primary";
        }

        if (auto sr = eo.find("sorting_rank");
            sr != eo.end() && sr->value().is_number()) {
          if (sr->value().is_int64()) {
            entry.sorting_rank = sr->value().as_int64();
          } else {
            entry.sorting_rank = sr->value().as_uint64();
          }
        }

        if (auto dpi = eo.find("data_processing_id");
            dpi != eo.end() && dpi->value().is_string()) {
          entry.data_processing_id = dpi->value().as_string();
        }

        if (auto tr = eo.find("transform");
            tr != eo.end() && tr->value().is_string()) {
          entry.transform = tr->value().as_string();
        }

        entries_.push_back(std::move(entry));
      }
    }
  }

  std::ranges::sort(entries_, {}, &Entry::array_name);
}

/******************************************************************************/
EntityType ArrayIndex::entity_type() const { return entity_type_; }

/******************************************************************************/
const std::string& ArrayIndex::prefix() const { return prefix_; }

/******************************************************************************/
const std::vector<ArrayIndex::Entry>& ArrayIndex::entries() const
{
  return entries_;
}

/******************************************************************************/
std::vector<ArrayIndex::Entry> ArrayIndex::entries(PSI::ArrayType type) const
{
  return entries_ |
         std::views::filter([type](const auto& c) { return c.array_type == type; }) |
         std::ranges::to<std::vector>();
}

/******************************************************************************/
std::vector<ArrayIndex::Entry> ArrayIndex::entries(const Data::Dimension& d) const
{
  return entries_ | std::views::filter([d](const auto& c) {
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

  std::vector<Entry> cols;
  cols.reserve(entries_.size()); // Should be small

  std::ranges::unique_copy(entries_.begin(), entries_.end(),
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
std::optional<Schema::Column> ArrayIndex::entry_column(const Schema::StructMap& map,
                                                       const Entry& col)
{
  auto it = map.find(prefix_);
  if (it == map.end()) return {};

  auto field = it->second->field(col.name);
  if (!field.has_value()) return {};

  return std::make_pair(it->second, field.value());
}

} // namespace MzPeak::Data
