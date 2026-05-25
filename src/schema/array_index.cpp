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
ArrayIndex::Array make_index(EntityType entity_type, const std::string& prefix) {
  ArrayIndex::Array index;

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
    : entity_type_(entity_type), prefix_(obj.at("prefix").as_string()) {
  auto entries = obj.find("entries");

  if (entries != obj.end() && entries->value().is_array()) {
    auto entries_ary(entries->value().as_array());
    arrays_.reserve(entries_ary.size() + 1);

    // The first array entry is actually the index itself.
    arrays_.push_back(make_index(entity_type, prefix_));

    for (const auto& entry : entries_ary) {
      if (entry.is_object()) {
        const auto& eo(entry.as_object());
        Array array;
        array.array_name = eo.at("array_name").as_string();
        array.buffer_format =
            buffer_format_from_string(eo.at("buffer_format").as_string());
        array.context = entity_type_from_string(eo.at("context").as_string());
        array.path = eo.at("path").as_string();
        array.data_type = PSI::data_type_from_string(eo.at("data_type").as_string());
        array.array_type =
            PSI::array_type_from_string(eo.at("array_type").as_string());
        array.unit = eo.at("unit").as_string();

        if (auto bp = eo.find("buffer_priority");
            bp != eo.end() && bp->value().is_string()) {
          array.buffer_priority = bp->value().as_string() == "primary";
        }

        if (auto sr = eo.find("sorting_rank");
            sr != eo.end() && sr->value().is_number()) {
          if (sr->value().is_int64()) {
            array.sorting_rank = sr->value().as_int64();
          } else {
            array.sorting_rank = sr->value().as_uint64();
          }
        }

        if (auto dpi = eo.find("data_processing_id");
            dpi != eo.end() && dpi->value().is_string()) {
          array.data_processing_id = dpi->value().as_string();
        }

        if (auto tr = eo.find("transform");
            tr != eo.end() && tr->value().is_string()) {
          array.transform = tr->value().as_string();
        }

        arrays_.push_back(std::move(array));
      }
    }
  }
}

/******************************************************************************/
EntityType ArrayIndex::entity_type() const { return entity_type_; }

/******************************************************************************/
const std::string& ArrayIndex::prefix() const { return prefix_; }

/******************************************************************************/
const std::vector<ArrayIndex::Array>& ArrayIndex::arrays() const { return arrays_; }

/******************************************************************************/
std::vector<ArrayIndex::Array> ArrayIndex::arrays(PSI::ArrayType type) const {
  return arrays_ |
         std::views::filter([type](const auto& a) { return a.array_type == type; }) |
         std::ranges::to<std::vector>();
}

/******************************************************************************/
void ArrayIndex::num_entities(const std::optional<std::size_t>& ne) {
  num_entities_ = ne;
}

/******************************************************************************/
std::optional<std::size_t> ArrayIndex::num_entities() const { return num_entities_; }

/******************************************************************************/
std::optional<int> ArrayIndex::column_index(const Array& array) const {
  auto it = array_map_.find(array.path);

  if (it == array_map_.end()) {
    return {};
  } else {
    return it->second;
  }
}

/******************************************************************************/
void ArrayIndex::array_map(ArrayMap array_map) { array_map_ = std::move(array_map); }

} // namespace MzPeak::Schema
