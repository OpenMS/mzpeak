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
#include "mzpeak/data/transformer/primary.h"
#include "mzpeak/data/transformer/secondary.h"
#include "mzpeak/exception.h"
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
  void decode_with_nulls(const ArrayIndex::Dimension&,
                         const N& null_decoder,
                         std::vector<V>&) const;

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
  Util::Type type = dim.type_or_throw();

  if (type == Util::Type::Float32) {
    remap<Util::Type::Float32>(dim, v);
  } else if (type == Util::Type::Float64) {
    remap<Util::Type::Float64>(dim, v);
  } else {
    Util::lift_type(type, []<Util::Type X> {
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
  Util::Type type = dim.type_or_throw();

  if (type == Util::Type::Int32) {
    remap<Util::Type::Int32>(dim, v);
  } else if (type == Util::Type::Int64) {
    remap<Util::Type::Int64>(dim, v);
  } else {
    Util::lift_type(type, []<Util::Type X> {
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
    v.insert(v.end(), tmp.begin(), tmp.end());
  }
}

/******************************************************************************/
template <typename T>
template <typename V>
void Decoder<T>::decode(const ArrayIndex::Dimension& dim, std::vector<V>& v) const
{
  switch (signals_->array_index()->layout()) {
  case ArrayIndex::Layout::Point:
  case ArrayIndex::Layout::Chunked:
    if (dim.needs_delta_model()) {
      using N = NullMarking::Decoder<V, T>;
      decode_with_nulls<N, V>(dim, N{delta_estimator_}, v);
    } else {
      using N = Util::Decoders::NullToZero<V>;
      decode_with_nulls<N, V>(dim, N{}, v);
    }
    break;

  case ArrayIndex::Layout::Unknown:
    throw UnknownLayoutError("cannot decode dimension: " + dim.name);
  }
}

/******************************************************************************/
template <typename T>
template <typename N, typename V>
void Decoder<T>::decode_with_nulls(const ArrayIndex::Dimension& dim,
                                   const N& null_decoder,
                                   std::vector<V>& v) const
{
  const auto& primary_entry = dim.values_entry();
  auto col = signals_->column(primary_entry);

  if (!col.has_value()) {
    throw ParquetError("unable to decode dimension, not in schema: " + dim.name);
  } else if (!slice_->has_column(col.value())) {
    return; // No data to decode so we can exit early.
  }

  auto go = [&](auto&& decoder) -> void { slice_->array(col.value(), v, decoder); };

  if (primary_entry.buffer_format == Schema::BufferFormat::Point) {
    auto decoder = Util::Decoders::Scalar<V, std::vector<V>, N>(null_decoder);
    go(decoder);
  } else {
    if (dim.is_main_axis()) {
      using Transformer = Transformer::Primary::Decoder<V>;
      Transformer transformer(signals_, slice_, dim);
      auto decoder = Util::Decoders::Flattened<V, std::vector<V>, N, Transformer>(
          null_decoder, std::move(transformer));
      go(decoder);
    } else {
      using Transformer = Transformer::Secondary::Decoder<V>;
      Transformer transformer(dim);
      auto decoder = Util::Decoders::Flattened<V, std::vector<V>, N, Transformer>(
          null_decoder, std::move(transformer));
      go(decoder);
    }
  }
}

} // namespace MzPeak::Data::Encoding
