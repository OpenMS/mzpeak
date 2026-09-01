/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <utility>

#include "mzpeak/schema/entity_type.h"

namespace MzPeak::Schema {

/******************************************************************************/
std::string entity_type_to_string(EntityType::Type t)
{
  using enum EntityType::Type;

  switch (t) {
  case Spectrum:
    return "spectrum";
  case Chromatogram:
    return "chromatogram";
  case WavelengthSpectrum:
    return "wavelength spectrum";
  }

  std::unreachable();
}

/******************************************************************************/
EntityType::value_type entity_type_from_string(std::string_view s)
{
  using enum EntityType::Type;

  if (s == "spectrum") {
    return Spectrum;
  } else if (s == "chromatogram") {
    return Chromatogram;
  } else if (s == "wavelength spectrum") {
    return WavelengthSpectrum;
  } else {
    return std::string(s);
  }
}

/******************************************************************************/
EntityType::EntityType(std::string_view s)
    : val_(entity_type_from_string(s))
{
}

/******************************************************************************/
EntityType::EntityType(Type t)
    : val_(t)
{
}

/******************************************************************************/
std::string EntityType::to_string() const
{
  return std::visit(
      [](auto&& v) {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, Type>) {
          return entity_type_to_string(v);
        } else {
          return v;
        }
      },
      val_);
}

/******************************************************************************/
std::optional<EntityType::Type> EntityType::type() const
{
  return std::visit(
      [](auto&& v) -> std::optional<EntityType::Type> {
        using T = std::decay_t<decltype(v)>;

        if constexpr (std::is_same_v<T, Type>) {
          return v;
        } else {
          return std::nullopt;
        }
      },
      val_);
}

/******************************************************************************/
std::string EntityType::index_column_name() const { return to_string() + "_index"; }

/******************************************************************************/
std::string EntityType::array_index_name() const
{
  return to_string() + "_array_index";
}

/******************************************************************************/
std::string EntityType::metadata_count_key() const { return to_string() + "_count"; }

} // namespace MzPeak::Schema
