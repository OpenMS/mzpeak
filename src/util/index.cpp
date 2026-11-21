/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/util/index.h"

namespace MzPeak::Util {

/******************************************************************************/
void tag_invoke(const boost::json::value_from_tag&,
                boost::json::value& jv,
                const Index::Node& node)
{
  boost::json::array rgs(node.row_groups_.begin(), node.row_groups_.end());
  jv = boost::json::object{{"min", avt_to_json(node.min_value_)},
                           {"max", avt_to_json(node.max_value_)},
                           {"rgs", rgs}};
}

/******************************************************************************/
void tag_invoke(const boost::json::value_from_tag&,
                boost::json::value& jv,
                const Index& idx)
{
  boost::json::object obj;

  for (const auto& col : idx.get()) {
    obj.emplace(std::to_string(col.first), boost::json::value_from(*col.second));
  }

  jv = obj;
}

/******************************************************************************/
Index::Index()
    : column_index_()
{
}

/******************************************************************************/
Index::key_type Index::to_key(const Schema::Column& col) const
{
  return col.second->absolute_index();
}

/******************************************************************************/
void Index::insert(const Schema::Column& col,
                   const any_value_type& min,
                   const any_value_type& max,
                   row_group_id rg)
{
  return insert(to_key(col), min, max, rg);
}

/******************************************************************************/
void Index::insert(key_type key,
                   const any_value_type& min,
                   const any_value_type& max,
                   row_group_id rg)
{
  auto overlapping_p = [&min, &max](const auto& node) -> bool {
    return (node.min_value_ <= min && node.max_value_ >= min) ||
           (node.min_value_ <= max && node.max_value_ >= max);
  };

  auto loc = column_index_.find(key);

  if (loc == column_index_.end()) {
    // The map doesn't have this column indexed yet.
    Node n{min, max, {rg}};
    auto v = std::make_shared<std::vector<Node>>();
    v->push_back(std::move(n));
    column_index_[key] = v;
  } else {
    // The map has this column so we need to find the correct bin.
    // That is, a bit that is *near* the one being inserted so we can
    // check if the two should be merged.
    auto bin = std::ranges::lower_bound(*loc->second, min, {}, &Node::max_value_);

    if (bin != loc->second->end() && overlapping_p(*bin)) {
      // The found node overlaps with with the one we want to insert,
      // so we have to merge the two.
      bin->min_value_ = std::min(bin->min_value_, min);
      bin->max_value_ = std::max(bin->max_value_, max);
      bin->row_groups_.insert(rg);
    } else {
      // `bin` is the end of the vector or we need to insert a new
      // Node prior to the found bin.
      Node n{min, max, {rg}};
      loc->second->insert(bin, std::move(n));
    }
  }
}

/******************************************************************************/
const std::map<Index::key_type, Index::value_type>& Index::get() const
{
  return column_index_;
}

/******************************************************************************/
boost::json::value Index::to_json() const { return boost::json::value_from(*this); }

} // namespace MzPeak::Util
