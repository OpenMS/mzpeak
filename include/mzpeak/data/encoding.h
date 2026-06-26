/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <arrow/array.h>
#include <vector>

#include "mzpeak/data/array_index.h"
#include "mzpeak/data/dimension.h"
#include "mzpeak/schema/group.h"
#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/util/slice.h"

namespace MzPeak::Data {

/**
 * Decode mzPeak signal data encoding (point and chunk).
 */
template <Schema::PSI::DataType T> class Encoding {
public:
  /// Values that are encoded/decoded by this object.
  using value_type = typename Schema::PSI::data_type_traits<T>::value_type;

  /// Constructor.
  Encoding(std::shared_ptr<ArrayIndex> array_index,
           std::shared_ptr<Schema::GroupMap> group_map,
           std::shared_ptr<Util::Slice> slice)
      : array_index_(std::move(array_index))
      , group_map_(std::move(group_map))
      , slice_(std::move(slice))
  {
  }

  /// Destructor.
  ~Encoding() = default;

  /**
   * Decode a single dimension.
   */
  void decode_dimension(const Dimension&, std::vector<value_type>&) const;

  /**
   * Decode a dimension using the "point" encoding.
   *
   * You probably want to use `decode_dimension` instead.
   */
  void decode_point(const Schema::Column&, std::vector<value_type>&) const;

  /**
   * Decode a dimension using the "chunked" encoding.
   *
   * You probably want to use `decode_array` instead.
   */
  // std::vector<value_type>
  // decode_chunked(const std::vector<Schema::ArrayIndex::Array>&) const;

private:
  std::shared_ptr<ArrayIndex> array_index_;
  std::shared_ptr<Schema::GroupMap> group_map_;
  std::shared_ptr<Util::Slice> slice_;
};

/******************************************************************************/
template <Schema::PSI::DataType T>
void Encoding<T>::decode_dimension(
    const Dimension& dim, std::vector<typename Encoding<T>::value_type>& v) const
{
  auto columns = array_index_->entries(dim);

  if (columns.empty()) {
    std::string msg("unable to decode dimension, wrong encoding: ");
    throw ParquetError(msg + dim.name);
  } else if (columns.size() == 1 &&
             columns[0].buffer_format == Schema::BufferFormat::Point) {
    auto field = array_index_->entry_column(*group_map_, columns[0]);

    if (!field.has_value()) {
      throw ParquetError("unable to decode dimension, not in schema: " + dim.name);
    }

    decode_point(field.value(), v);
  } else {
    throw("not implemented");
    // return decode_chunked(arrays);
  }
}

/******************************************************************************/
template <Schema::PSI::DataType T>
void Encoding<T>::decode_point(
    const Schema::Column& col,
    std::vector<typename Encoding<T>::value_type>& v) const
{
  // FIXME: use null mark decoding for the main axis and zeros for
  // other dimensions.
  auto on_null = [](auto& array, auto index) -> std::optional<value_type> {
    return 0;
  };

  slice_->array(col, v, Util::Decoders::Scalar<value_type>(on_null));
}

} // namespace MzPeak::Data
