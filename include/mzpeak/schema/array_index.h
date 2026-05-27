/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <boost/json.hpp>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "mzpeak/schema/buffer_format.h"
#include "mzpeak/schema/entity_type.h"
#include "mzpeak/schema/psi/array_type.h"
#include "mzpeak/schema/psi/data_type.h"

namespace MzPeak::Schema {

namespace json = boost::json;

/**
 * Description of the data found in a data file.
 */
class ArrayIndex final {
public:
  /// Mapping from column path in the schema to the column index.
  using ColumnMap = std::map<std::string, int>;

  /**
   * A type to describe each entry in the index.
   */
  struct Column {
    /// The name of the array being described. If this is an
    /// MS:1000786|non-standard array, this should be the descriptive
    /// name for the array, otherwise it should be the human-readable
    /// name for the `array_type` from the PSI-MS controlled
    /// vocabulary.
    std::string array_name;

    /// How the array data is stored in the Parquet file.
    BufferFormat buffer_format;

    /// The entity type this column belongs to.
    EntityType context = EntityType::Other;

    /// The path from the *root* of the Parquet file's schema to this
    /// column.
    std::string path;

    /// The data type for this column, denoted using a CURIE from the
    /// PSI-MS controlled vocabulary for a child of MS:1000518 (binary
    /// data type).
    PSI::DataType data_type = PSI::DataType::Float64;

    /// The type of column this is.
    PSI::ArrayType array_type = PSI::ArrayType::NonStandard;

    /// The unit describing the measurement, denoted using a CURIE
    /// from the PSI-MS controlled vocabulary or the unit ontology.
    std::string unit;

    /// A flag to indicate this column is the representative instance
    /// of this column type. The primary column of its type SHOULD have
    /// a simplified name, otherwise the writer should make it as
    /// unique as possible without sacrificing readability.
    bool buffer_priority = false;

    /// What order, following the entity index, this column was sorted
    /// in ascending order if any. The lower the rank, the earlier the
    /// dimension was sorted, starting from 0. If this value is null
    /// or absent, this column is assumed not to be sorted.
    std::optional<std::size_t> sorting_rank;

    /// The identifier of a data processing method that governs this
    /// column. If not specified, assumed to be the default data
    /// processing method for this run.
    std::optional<std::string> data_processing_id;

    /// A transformation that may be applied to this column such as
    /// zero trimming and null marking or Numpress compression,
    /// denoted as a CURIE from the PSI-MS controlled vocabulary. Some
    /// values are only usable with the chunked layout.
    std::optional<std::string> transform;
  };

  /// Default constructor.
  ArrayIndex() = default;

  /// Construct from JSON.
  explicit ArrayIndex(EntityType, const json::object&);

  /// Destructor.
  ~ArrayIndex() = default;

  /**
   * Get the EntityType for the containing data file.
   */
  EntityType entity_type() const;

  /**
   * Get the path to the root node.
   */
  const std::string& prefix() const;

  /**
   * Get a list of column definitions.
   */
  const std::vector<Column>& columns() const;

  /**
   * Get a list of column definitions that are for the given array
   * type.
   */
  std::vector<Column> columns(PSI::ArrayType) const;

  /**
   * Update the hint as to how many entities are in the data file.
   */
  void num_entities(const std::optional<std::size_t>&);

  /**
   * Get a hint as to how many entities there are in the data file.
   */
  std::optional<std::size_t> num_entities() const;

  /**
   * Return the column index for the given column.
   */
  std::optional<int> column_index(const Column&) const;

  /**
   * Update the column map;
   */
  void column_map(ColumnMap column_map);

private:
  // The entity type for the entire Parquet file.
  EntityType entity_type_;

  // Root node.
  std::string prefix_ = "point";

  // Entries;
  std::vector<Column> columns_;

  // We might know how many entities are in the table.
  std::optional<std::size_t> num_entities_;

  // Column -> column index.
  ColumnMap column_map_;
};

} // namespace MzPeak::Schema
