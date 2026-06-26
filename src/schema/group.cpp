/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <memory>
#include <parquet/schema.h>
#include <parquet/types.h>
#include <ranges>

#include "mzpeak/schema/group.h"
#include "mzpeak/schema/psi/data_type.h"

namespace MzPeak::Schema {

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
std::pair<Group::Field::Kind, std::optional<PSI::DataType>>
field_type_from_parquet(const std::shared_ptr<parquet::schema::GroupNode>& node)
{
  switch (node->logical_type()->type()) {
  case parquet::LogicalType::Type::LIST:
    // If the list is a homogeneous collection of scalars...
    if (auto prim = find_homogeneous_primitive(
            std::static_pointer_cast<parquet::schema::Node>(node));
        prim != nullptr) {
      return std::make_pair(Group::Field::Kind::List,
                            PSI::data_type_from_parquet(*prim));
    } else {
      return std::make_pair(Group::Field::Kind::List, std::nullopt);
    }
  default:
    return std::make_pair(Group::Field::Kind::Unknown, std::nullopt);
  }
}

/******************************************************************************/
Group::Field::Field(const std::string_view& column_name,
                    index_type rel_index,
                    index_type abs_index)
    : rel_index_(rel_index)
    , abs_index_(abs_index)
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
    cv_type_ = CVType(*name_begin, *(name_begin + 1));
    name_begin += 2;
  }

  auto unit = std::ranges::find_last(name_begin, name_end, "unit"sv);

  if (unit.begin() != name_begin && unit.begin() != name_end &&
      std::ranges::distance(unit.begin(), name_end) == 3) {
    cv_unit_ = CVUnit(*(unit.begin() + 1), *(unit.begin() + 2));
    name_end = unit.begin();
  }

  clean_name_ = join_(name_begin, name_end);
}

/******************************************************************************/
Group::index_type Group::Field::relative_index() const { return rel_index_; }

/******************************************************************************/
Group::index_type Group::Field::absolute_index() const { return abs_index_; }

/******************************************************************************/
const std::string& Group::Field::name() const { return clean_name_; }

/******************************************************************************/
const std::string& Group::Field::schema_name() const { return schema_name_; }

/******************************************************************************/
Group::Field::Kind Group::Field::kind() const { return kind_; }

/******************************************************************************/
const std::optional<Group::CVType>& Group::Field::cv_type() const
{
  return cv_type_;
}

/******************************************************************************/
const std::optional<Group::CVUnit>& Group::Field::cv_unit() const
{
  return cv_unit_;
}

/******************************************************************************/
const std::optional<PSI::DataType>& Group::Field::data_type() const
{
  return data_type_;
}

/******************************************************************************/
void Group::Field::data_type(PSI::DataType dt) { data_type_ = dt; }

/******************************************************************************/
Group::Group(const parquet::schema::GroupNode& node,
             index_type index,
             index_type offset)
    : name_(node.name())
    , index_(index)
{
  for (index_type i : std::views::iota(0, node.field_count())) {
    auto child = node.field(i);

    std::shared_ptr<Field> field =
        std::make_shared<Field>(child->name(), i, offset + i);

    if (child->is_primitive()) {
      auto prim = std::static_pointer_cast<parquet::schema::PrimitiveNode>(child);
      field->kind_ = Field::Kind::Scalar;
      field->data_type_ = PSI::data_type_from_parquet(*prim);
    } else {
      auto grp = std::static_pointer_cast<parquet::schema::GroupNode>(child);

      if (field->name() == "parameters" && grp->logical_type()->is_list()) {
        field->kind_ = Group::Field::Kind::Params;
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
const std::string& Group::name() const { return name_; }

/******************************************************************************/
Group::index_type Group::index() const { return index_; }

/******************************************************************************/
std::optional<std::shared_ptr<const Group::Field>>
Group::field(const std::string_view&& name) const
{
  auto it = fields_.find(std::string{name});

  if (it != fields_.end()) {
    return it->second;
  } else {
    return {};
  }
}

/******************************************************************************/
std::optional<std::shared_ptr<const Group::Field>>
Group::field(const CVType&& cvt) const
{
  const auto fields = fields_ | std::views::values;

  auto it =
      std::ranges::find_if(fields, [cvt](const std::shared_ptr<Field>& f) -> bool {
        return f->cv_type().has_value() && (f->cv_type().value() == cvt);
      });

  if (it == std::ranges::end(fields)) {
    return {};
  } else {
    return *it;
  }
}

/******************************************************************************/
const Group::field_map_t& Group::fields() const { return fields_; }

} // namespace MzPeak::Schema
