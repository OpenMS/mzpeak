/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <algorithm>
#include <ranges>

#include "mzpeak/data/array_index.h"
#include "mzpeak/schema/buffer_format.h"
#include "mzpeak/schema/entity_type.h"

namespace MzPeak::Data {

/******************************************************************************/
bool ArrayIndex::Dimension::needs_delta_model() const
{
  bool from_tansform = transform.has_value() && transform->needs_delta_model();
  return from_tansform || std::ranges::any_of(entries, [](const auto& e) {
           return e.sorting_rank.has_value() && e.sorting_rank.value() == 0;
         });
}

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
          std::optional<Schema::CV> maybe_cv =
              Schema::CV::from_string(tr->value().as_string());

          entry.transform = maybe_cv.and_then(
              [](const auto& cv) -> std::optional<Schema::PSI::Transform> {
                return Schema::PSI::Transform(cv);
              });
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
void ArrayIndex::num_entities(const std::optional<std::size_t>& ne)
{
  num_entities_ = ne;
}

/******************************************************************************/
std::optional<std::size_t> ArrayIndex::num_entities() const { return num_entities_; }

/******************************************************************************/
std::vector<ArrayIndex::Dimension> ArrayIndex::dimensions() const
{
  std::vector<std::vector<Entry>> groups =
      entries_ | std::views::chunk_by([](auto& a, auto& b) {
        return a.array_name == b.array_name && a.data_type == b.data_type &&
               a.array_type == b.array_type;
      }) |
      std::ranges::to<std::vector<std::vector<Entry>>>();

  std::vector<Dimension> result;
  result.reserve(groups.size());

  for (auto& group : groups) {
    if (group.empty()) continue;
    auto& head = group[0];

    result.push_back({head.name, head.data_type, head.array_type, head.transform,
                      std::move(group)});
  }

  return result;
}

/******************************************************************************/
std::optional<Schema::Column> ArrayIndex::entry_column(const Schema::GroupMap& map,
                                                       const Entry& col)
{
  auto it = map.find(prefix_);
  if (it == map.end()) return {};

  auto field = it->second->field(col.name);
  if (!field.has_value()) return {};

  return std::make_pair(it->second, field.value());
}

} // namespace MzPeak::Data
