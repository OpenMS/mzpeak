/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <memory>
#include <numeric>
#include <parquet/schema.h>
#include <parquet/types.h>
#include <ranges>

#include "mzpeak/schema/group.h"
#include "mzpeak/schema/psi/data_type.h"

namespace MzPeak::Schema {

/******************************************************************************/
template <typename T> std::optional<T> string_to_cv_child(std::string_view s)
{
  return CV::from_string(s).and_then([](const auto& cv) -> std::optional<T> {
    return T(cv.code(), cv.accession());
  });
}

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
bool nested(Group::Field::Kind k)
{
  using enum Group::Field::Kind;

  switch (k) {
  case Scalar:
    return false;
  case List:
    return false;
  case ListOfGroups:
    return true;
  case Group:
    return true;
  case Unknown:
    return false;
  }

  std::unreachable();
}

/******************************************************************************/
/// Remove nesting until you get to the bottom group.
std::shared_ptr<parquet::schema::GroupNode>
resolve_group(std::shared_ptr<parquet::schema::GroupNode> node)
{
  while (node->field_count() == 1 && node->field(0)->is_group()) {
    node = std::static_pointer_cast<parquet::schema::GroupNode>(node->field(0));
  }

  return node;
}

/******************************************************************************/
// Try to find a nested primitive node assuming that all fields will
// use the same type (i.e. homogeneous list).
std::shared_ptr<parquet::schema::PrimitiveNode>
find_homogeneous_primitive(std::shared_ptr<parquet::schema::Node> node)
{
  while (node != nullptr && node->is_group()) {
    auto group = std::static_pointer_cast<parquet::schema::GroupNode>(node);

    if (group->field_count() == 1) {
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
std::pair<Group::Field::Kind, std::optional<Util::Type>>
field_type_from_parquet(const std::shared_ptr<parquet::schema::GroupNode>& node)
{
  if (node->logical_type()->is_list()) {
    // If the list is a homogeneous collection of scalars...
    if (auto prim = find_homogeneous_primitive(
            std::static_pointer_cast<parquet::schema::Node>(node));
        prim != nullptr) {
      return std::make_pair(Group::Field::Kind::List,
                            Util::type_from_parquet(*prim));
    }

    return std::make_pair(Group::Field::Kind::ListOfGroups, std::nullopt);
  }

  return std::make_pair(Group::Field::Kind::Group, std::nullopt);
}

/******************************************************************************/
Group::Field::Field(std::string_view column_name,
                    index_type rel_index,
                    index_type abs_index)
    : rel_index_(rel_index)
    , abs_index_(abs_index)
    , schema_name_(column_name)
    , cv_type_()
    , cv_unit_()
    , type_()
{
}

/******************************************************************************/
Group::index_type Group::Field::relative_index() const { return rel_index_; }

/******************************************************************************/
Group::index_type Group::Field::absolute_index() const { return abs_index_; }

/******************************************************************************/
const std::string& Group::Field::name() const { return schema_name_; }

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
const std::optional<Util::Type>& Group::Field::type() const { return type_; }

/******************************************************************************/
void Group::Field::type(Util::Type type) { type_ = type; }

/******************************************************************************/
std::shared_ptr<Group> Group::Field::nested() const { return nested_; }

/******************************************************************************/
std::optional<Util::Numpress::Type> Group::Field::possibly_numpress() const
{
  return Util::Numpress::type_from_column_name(schema_name_);
}

/******************************************************************************/
Group::Group(const parquet::schema::GroupNode& node, const Schema::File& file)
    : name_("root")
    , is_root_(true)
    , index_(0)
    , fields_()
{
  // This is the root group so only collect non-group top-level columns.
  make_fields(node, file, 0);
}

/******************************************************************************/
Group::Group(const parquet::schema::GroupNode& node,
             const Schema::File& file,
             index_type index,
             index_type offset)
    : name_(node.name())
    , is_root_(false)
    , index_(index)
    , fields_()
{
  make_fields(node, file, offset);
}

/******************************************************************************/
void Group::make_fields(const parquet::schema::GroupNode& node,
                        const Schema::File& file,
                        index_type offset)
{

  auto link = [&](std::shared_ptr<Field>& field) -> void {
    fields_[field->name()] = field;

    std::string col_path = path(*field);
    const auto it = std::ranges::find(file.columns(), col_path, &File::Column::path);

    if (it != file.columns().end()) {
      field->cv_type_ = it->accession.and_then(&string_to_cv_child<CVType>);
      field->cv_unit_ = it->unit.and_then(&string_to_cv_child<CVUnit>);
    }
  };

  index_type rel{};

  auto new_field = [&](const auto& name) -> std::shared_ptr<Field> {
    auto field = std::make_shared<Field>(name, rel, rel + offset);
    link(field);
    ++rel;
    return field;
  };

  auto nest = [&](auto& field, auto& group) {
    auto nested = resolve_group(group);
    auto ri = field->relative_index();
    auto ai = field->absolute_index();
    field->nested_ = std::make_shared<Group>(*nested, file, ri, ai);
    field->nested_->name(field->name());
    offset += field->nested_->size() - 1;
  };

  for (index_type i : std::views::iota(0, node.field_count())) {
    auto child = node.field(i);

    if (child->is_primitive()) {
      auto field(new_field(child->name()));
      auto prim = std::static_pointer_cast<parquet::schema::PrimitiveNode>(child);
      field->kind_ = Field::Kind::Scalar;
      field->type_ = Util::type_from_parquet(*prim);
    } else if (child->is_group()) {
      auto grp = std::static_pointer_cast<parquet::schema::GroupNode>(child);
      auto grp_type = field_type_from_parquet(grp);
      auto field = new_field(grp->name());
      field->kind_ = grp_type.first;
      field->type_ = grp_type.second;
      if (nested(field->kind_)) nest(field, grp);
    } else {
      std::string msg(child->name());
      msg += " is neither a primitive nor a group";
      throw ParquetError("invalid schema: " + msg);
    }
  }
}

/******************************************************************************/
const std::string& Group::name() const { return name_; }

/******************************************************************************/
void Group::name(std::string_view s) { name_ = s; }

/******************************************************************************/
Group::index_type Group::index() const { return index_; }

/******************************************************************************/
std::optional<std::shared_ptr<const Group::Field>>
Group::field(std::string_view name) const
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

/******************************************************************************/
Group::field_map_t& Group::fields() { return fields_; }

/******************************************************************************/
std::size_t Group::size() const
{
  std::size_t count{};

  for (const auto& field : fields_) {
    if (field.second->nested_) {
      count += field.second->nested_->size();
    } else {
      ++count;
    }
  }

  return count;
}

/******************************************************************************/
bool Group::is_root() const { return is_root_; }

/******************************************************************************/
std::string Group::path(const Field& field) const
{
  if (is_root_) {
    return field.name();
  } else {
    return name() + "." + field.name();
  }
}

/******************************************************************************/
std::vector<Column> flatten_group_map(const GroupMap& gm)
{
  std::vector<Column> res;

  auto add = [&res](this auto self, const std::shared_ptr<Group>& group) -> void {
    for (const auto& field : group->fields()) {
      if (field.second->nested()) {
        self(field.second->nested());
      } else {
        res.push_back({group, field.second});
      }
    }
  };

  res.reserve(std::accumulate(gm.begin(), gm.end(), 0, [](auto acc, auto f) {
    return acc + f.second->size();
  }));

  for (const auto& group : gm) {
    add(group.second);
  }

  std::ranges::sort(res, {},
                    [](const auto& c) { return c.second->absolute_index(); });

  // Spot check:
  if (!res.empty()) {
    Group::index_type last = static_cast<Group::index_type>(res.size() - 1);

    if (res[0].second->absolute_index() != 0 ||
        res[last].second->absolute_index() != last) {
      throw ParquetError(
          "while flattening schema: column indices are not sequential");
    }
  }

  return res;
}

} // namespace MzPeak::Schema
