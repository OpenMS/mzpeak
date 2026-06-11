/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <memory>
#include <ranges>

#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/util/struct.h"
#include "parquet/schema.h"

namespace MzPeak::Util {

/******************************************************************************/
// clang doesn't support views::join yet :(
std::string join_(auto begin, auto end)
{
  std::string res(*begin);

  for (++begin; begin != end; ++begin) {
    res += "_" + *begin;
  }

  return res;
}

/******************************************************************************/
// Try to find a nested primitive node assuming that all fields will
// use the same type (i.e. homogeneous list).
std::shared_ptr<parquet::schema::PrimitiveNode>
find_homogeneous_primitive(std::shared_ptr<parquet::schema::Node> node)
{
  while (node != nullptr && node->is_group()) {
    auto group = std::static_pointer_cast<parquet::schema::GroupNode>(node);

    if (group->field_count() > 0) {
      node = group->field(0);
    } else {
      node = nullptr;
    }
  }

  if (node != nullptr && node->is_primitive()) {
    return std::static_pointer_cast<parquet::schema::PrimitiveNode>(node);
  } else {
    return nullptr;
  }
}

/******************************************************************************/
std::pair<Struct::Field::Kind, std::optional<Schema::PSI::DataType>>
field_type_from_parquet(const std::shared_ptr<parquet::schema::GroupNode>& node)
{
  switch (node->logical_type()->type()) {
  case parquet::LogicalType::Type::LIST:
    // If the list is a homogeneous collection of scalars...
    if (auto prim = find_homogeneous_primitive(
            std::static_pointer_cast<parquet::schema::Node>(node));
        prim != nullptr) {
      return std::make_pair(
          Struct::Field::Kind::List,
          Schema::PSI::data_type_from_parquet(prim->physical_type()));
    } else {
      return std::make_pair(Struct::Field::Kind::List, std::nullopt);
    }
  default:
    return std::make_pair(Struct::Field::Kind::Unknown, std::nullopt);
  }
}

/******************************************************************************/
Struct::Field::Field(const std::string_view& column_name, int column_index)
    : index_(column_index)
    , schema_name_(column_name)
{
  using std::operator""sv;

  auto tokens = column_name | std::views::split("_"sv) |
                std::ranges::to<std::vector<std::string>>();

  if (tokens.size() < 3) {
    clean_name_ = schema_name_;
    return;
  }

  auto name_begin = tokens.begin();
  auto name_end = tokens.end();

  if (*name_begin == "MS" || *name_begin == "UO") {
    cv_type_ = *name_begin + ":" + *(name_begin + 1);
    name_begin += 2;
  }

  auto unit = std::ranges::find_last(name_begin, name_end, "unit"sv);

  if (unit.begin() != name_begin && unit.begin() != name_end &&
      std::ranges::distance(unit.begin(), name_end) == 3) {
    cv_unit_ = *(unit.begin() + 1) + ":" + *(unit.begin() + 2);
    name_end = unit.begin();
  }

  clean_name_ = join_(name_begin, name_end);
}

/******************************************************************************/
int Struct::Field::index() const { return index_; }

/******************************************************************************/
const std::string& Struct::Field::name() const { return clean_name_; }

/******************************************************************************/
Struct::Field::Kind Struct::Field::kind() const { return kind_; }

/******************************************************************************/
std::optional<std::string> Struct::Field::cv_type() const { return cv_type_; }

/******************************************************************************/
std::optional<std::string> Struct::Field::cv_unit() const { return cv_unit_; }

/******************************************************************************/
const std::optional<Schema::PSI::DataType>& Struct::Field::data_type() const
{
  return data_type_;
}

/******************************************************************************/
Struct::Struct(const parquet::schema::GroupNode& node, int index)
    : name_(node.name())
    , index_(index)
{
  for (int i : std::views::iota(0, node.field_count())) {
    auto child = node.field(i);
    std::shared_ptr<Field> field = std::make_shared<Field>(child->name(), i);

    if (child->is_primitive()) {
      auto prim = std::static_pointer_cast<parquet::schema::PrimitiveNode>(child);
      field->kind_ = Field::Kind::Scalar;
      field->data_type_ = Schema::PSI::data_type_from_parquet(prim->physical_type());
    } else {
      auto grp = std::static_pointer_cast<parquet::schema::GroupNode>(child);

      if (field->name() == "parameters" && grp->logical_type()->is_list()) {
        field->kind_ = Struct::Field::Kind::Params;
      } else {
        auto grp_type = field_type_from_parquet(grp);
        field->kind_ = grp_type.first;
        field->data_type_ = grp_type.second;
      }
    }

    fields_[field->name()] = field;
  }
}

/******************************************************************************/
const std::string& Struct::name() const { return name_; }

/******************************************************************************/
int Struct::index() const { return index_; }

/******************************************************************************/
std::optional<std::reference_wrapper<const Struct::Field>>
Struct::field(const std::string_view& name) const
{
  auto it = fields_.find(std::string{name});

  if (it != fields_.end()) {
    return std::ref(*it->second);
  } else {
    return {};
  }
}

/******************************************************************************/
const Struct::field_map_t& Struct::fields() const { return fields_; }

} // namespace MzPeak::Util
