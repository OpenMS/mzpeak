/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>

#include "mzpeak/schema/psi/data_type.h"

// Forward declarations.
namespace parquet::schema {
class GroupNode;
} // namespace parquet::schema

namespace MzPeak::Schema {

/**
 * Internal representation of a parquet schema `Group` with fields.
 *
 * The goal is to flatten the parquet schema as much as possible given
 * what we know about the mzPeak schema.
 */
class Struct final {
public:
  /// Type used to store column indexes.
  using index_type = int32_t;

  /**
   * A possibly non-scalar field.
   */
  class Field final {
  public:
    enum class Kind {
      /// Elements are scalars values.
      Scalar,

      /// Elements are a list of scalars.
      List,

      /// Metadata parameters,
      Params,

      /// No clue.
      Unknown,
    };

    /// Constructor from an encoded column name.
    explicit Field(const std::string_view& column_name,
                   index_type rel_index,
                   index_type abs_index);

    /// Destructor.
    ~Field() = default;

    /**
     * Column index inside the parent struct.
     */
    index_type relative_index() const;

    /**
     * Column index inside the parquet file.
     */
    index_type absolute_index() const;

    /**
     * The name of this field using underscores to replace spaces and
     * other special characters.
     */
    const std::string& name() const;

    /**
     * The structural type this field represents.
     */
    Kind kind() const;

    /**
     * Controlled vocabulary code and accession for the field type.
     */
    std::optional<std::string> cv_type() const;

    /**
     * Controlled vocabulary code and accession for the field unit.
     */
    std::optional<std::string> cv_unit() const;

    /**
     * The data type for values in this field.
     */
    const std::optional<PSI::DataType>& data_type() const;

    /**
     * Update the field's data type.
     */
    void data_type(PSI::DataType);

  private:
    friend class Struct;

    index_type rel_index_;
    index_type abs_index_;
    std::string schema_name_;
    std::string clean_name_;
    std::optional<std::string> cv_type_;
    std::optional<std::string> cv_unit_;
    std::optional<PSI::DataType> data_type_;
    Kind kind_ = Kind::Scalar;
  };

public:
  /// Fields are stored in a map for quick look-up using their name.
  using field_map_t = std::map<std::string, std::shared_ptr<Field>>;

  /// Constructor from a parquet schema descriptor.
  explicit Struct(const parquet::schema::GroupNode&,
                  index_type index,
                  index_type offset);

  /// Destructor.
  ~Struct() = default;

  /**
   * The schema name for this struct.
   */
  const std::string& name() const;

  /**
   * Return the schema column index of this struct.
   */
  index_type index() const;

  /**
   * Find a field given it's name.
   *
   * NOTE: For metadata structs this is the cleaned name, not the raw
   * schema node name.
   */
  std::optional<std::shared_ptr<const Field>> field(const std::string_view&) const;

  /**
   * Return a map of all fields.
   */
  const field_map_t& fields() const;

private:
  std::string name_;
  index_type index_;
  field_map_t fields_;
};

/**
 * Mapping from struct name to a Struct.
 */
using StructMap = std::map<std::string, std::shared_ptr<Struct>>;

/**
 * A parquet column can be uniquely identified using its parent struct
 * and field.
 */
using Column =
    std::pair<std::shared_ptr<const Struct>, std::shared_ptr<const Struct::Field>>;

} // namespace MzPeak::Schema
