/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <cstdint>
#include <set>

#include "mzpeak/schema/group.h"
#include "mzpeak/util/types.h"

namespace MzPeak::Util {

/**
 * Internal index of a Parquet table.
 */
class Index final {
public:
  using row_group_id = int32_t;
  using matching_groups = std::set<row_group_id>;

  // Find a row group based on its min and max values.
  struct Node {
    any_value_type min_value_;
    any_value_type max_value_;
    matching_groups row_groups_;
  };

  // The vector of nodes is sorted by the `min_value_`.
  using value_type = std::shared_ptr<std::vector<Node>>;
  using key_type = Schema::Group::index_type;

  /// Constructor.
  Index();

  /// Convert a column into a table lookup key.
  key_type to_key(const Schema::Column& col) const;

  /// Record the given min and max values for the supplied row group.
  void insert(const Schema::Column&,
              const any_value_type&,
              const any_value_type&,
              row_group_id);

  /// Record the given min and max values for the supplied row group.
  void insert(key_type, const any_value_type&, const any_value_type&, row_group_id);

  /// Read-only access to the index.
  const std::map<key_type, value_type>& get() const;

  /// Convert this index into a JSON value.
  boost::json::value to_json() const;

private:
  // Multi-level index, the outer index is based on the column's
  // absolute parquet index and then a collection of Nodes to find the
  // row group.
  std::map<key_type, value_type> column_index_;
};

} // namespace MzPeak::Util
