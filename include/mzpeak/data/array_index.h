/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <boost/json.hpp>
#include <optional>
#include <string>
#include <vector>

#include "mzpeak/schema/buffer_format.h"
#include "mzpeak/schema/entity_type.h"
#include "mzpeak/schema/group.h"
#include "mzpeak/schema/psi/array_type.h"
#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/schema/psi/transform.h"

namespace MzPeak::Data {

namespace json = boost::json;
using namespace MzPeak::Schema;

/**
 * Description of the data found in a data file.
 */
class ArrayIndex final {
public:
  /**
   * A type to describe each entry in the index.
   */
  struct Entry {
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

    /// The name of the column with the schema prefix removed.
    std::string name;

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
    std::optional<Schema::PSI::Transform> transform;
  };

  /**
   * Describes a single dimension from the signals data file.
   */
  struct Dimension {
    /// The name of this dimension (e.g., "mz", "intensity", etc.)
    std::string name;

    /// Data type used for decodeing.
    Schema::PSI::DataType data_type;

    /// The type of elements stored in this dimension.
    Schema::PSI::ArrayType array_type;

    /// Transformation information for this dimension.
    std::optional<Schema::PSI::Transform> transform;

    /// The index entries that make up this dimension.
    std::vector<Entry> entries;

    /// Does this dimension need a delta model for decoding?
    bool needs_delta_model() const;
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
   * Get a list of entry definitions.
   */
  const std::vector<Entry>& entries() const;

  /**
   * Update the hint as to how many entities are in the data file.
   */
  void num_entities(const std::optional<std::size_t>&);

  /**
   * Get a hint as to how many entities there are in the data file.
   */
  std::optional<std::size_t> num_entities() const;

  /**
   * Return a list of all dimensions stored in the data file.
   */
  std::vector<Dimension> dimensions() const;

  /**
   * Convert an array index entry into a Group::Field;
   */
  std::optional<Schema::Column> entry_column(const Schema::GroupMap&, const Entry&);

private:
  // The entity type for the entire Parquet file.
  EntityType entity_type_;

  // Root node.
  std::string prefix_ = "point";

  // Entries;
  std::vector<Entry> entries_;

  // We might know how many entities are in the table.
  std::optional<std::size_t> num_entities_;
};

} // namespace MzPeak::Data
