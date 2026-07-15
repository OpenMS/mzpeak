/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <arrow/array.h>
#include <memory>
#include <vector>

#include "mzpeak/data/array_index.h"
#include "mzpeak/data/null_marking.h"
#include "mzpeak/data/signals.h"
#include "mzpeak/exception.h"
#include "mzpeak/schema/group.h"
#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/util/slice.h"
#include "mzpeak/util/types.h"

namespace MzPeak::Data::Encoding {

/**
 * Decode mzPeak signal data encoding (point and chunk).
 *
 * The template type T should match the type for the main axis.
 */
template <typename T> class Decoder {
public:
  /// Constructor.
  Decoder(std::shared_ptr<Signals> signals,
          std::shared_ptr<Util::Slice> slice,
          const Util::DeltaEstimator<T>& estimator)
      : signals_(std::move(signals))
      , slice_(std::move(slice))
      , delta_estimator_(estimator)
  {
  }

  /**
   * Decode a float or double.
   */
  template <typename V>
  void decimal(const ArrayIndex::Dimension&, std::vector<V>&) const;

  /**
   * Decode a 32- or 64-bit integer.
   */
  template <typename V>
  void integer(const ArrayIndex::Dimension&, std::vector<V>&) const;

private:
  template <typename V>
  void decode(const ArrayIndex::Dimension&, std::vector<V>&) const;

  template <typename N, typename V>
  void point(const Schema::Column&, const N& null_decoder, std::vector<V>&) const;

  template <Util::Type From, typename V>
  void remap(const ArrayIndex::Dimension& dim, std::vector<V>& v) const;

  std::shared_ptr<Signals> signals_;
  std::shared_ptr<Util::Slice> slice_;
  Util::DeltaEstimator<T> delta_estimator_;
};

/******************************************************************************/
template <typename T>
template <typename V>
void Decoder<T>::decimal(const ArrayIndex::Dimension& dim, std::vector<V>& v) const
{
  switch (dim.data_type) {
  case Schema::PSI::DataType::Float32: {
    remap<Util::Type::Float32>(dim, v);
  } break;
  case Schema::PSI::DataType::Float64:
    remap<Util::Type::Float64>(dim, v);
    break;
  default:
    Util::Type t = Schema::PSI::data_type_to_type(dim.data_type);
    Util::lift_type(t, []<Util::Type X> {
      std::string msg("Expected float or double but got: ");
      msg += Util::type_traits<X>::name;
      throw(TypeError(msg));
    });
  }
}

/******************************************************************************/
template <typename T>
template <typename V>
void Decoder<T>::integer(const ArrayIndex::Dimension& dim, std::vector<V>& v) const
{
  switch (dim.data_type) {
  case Schema::PSI::DataType::Int32: {
    remap<Util::Type::Int32>(dim, v);
  } break;
  case Schema::PSI::DataType::Int64:
    remap<Util::Type::Int64>(dim, v);
    break;
  default:
    Util::Type t = Schema::PSI::data_type_to_type(dim.data_type);
    Util::lift_type(t, []<Util::Type X> {
      std::string msg("Expected int32 or int64 but got: ");
      msg += Util::type_traits<X>::name;
      throw(TypeError(msg));
    });
  }
}

/******************************************************************************/
template <typename T>
template <Util::Type From, typename V>
void Decoder<T>::remap(const ArrayIndex::Dimension& dim, std::vector<V>& v) const
{
  using F = Util::type_traits<From>::value_type;

  if constexpr (std::is_same_v<F, V>) {
    decode<V>(dim, v);
  } else {
    std::vector<F> tmp;
    decode<F>(dim, tmp);
    v.reserve(tmp.size());
    v.insert(v.end(), tmp.begin(), tmp.end());
  }
}

/******************************************************************************/
template <typename T>
template <typename V>
void Decoder<T>::decode(const ArrayIndex::Dimension& dim, std::vector<V>& v) const
{
  const auto& entries = dim.entries;

  if (entries.empty()) {
    std::string msg("unable to decode dimension, wrong encoding: ");
    throw ParquetError(msg + dim.name);
  } else if (entries.size() == 1 &&
             entries[0].buffer_format == Schema::BufferFormat::Point) {

    auto field =
        signals_->array_index()->entry_column(*signals_->groups(), entries[0]);

    if (!field.has_value()) {
      throw ParquetError("unable to decode dimension, not in schema: " + dim.name);
    }

    if (dim.needs_delta_model()) {
      if constexpr (std::is_same_v<T, V>) {
        using N = NullMarking::Decoder<T>;
        point<N, V>(field.value(), N{delta_estimator_}, v);
      } else {
        throw TypeError("delta model only usable on the main axis");
      }
    } else {
      using N = Util::Decoders::NullToZero<V>;
      point<N, V>(field.value(), N{}, v);
    }
  } else {
    throw("not implemented");
    // return decode_chunked(arrays);
  }
}

/******************************************************************************/
template <typename T>
template <typename N, typename V>
void Decoder<T>::point(const Schema::Column& col,
                       const N& null_decoder,
                       std::vector<V>& v) const
{
  slice_->array(col, v, Util::Decoders::Scalar<V, std::vector<V>, N>(null_decoder));
}

} // namespace MzPeak::Data::Encoding
