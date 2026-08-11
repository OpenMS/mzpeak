/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <utility>

#include "mzpeak/data/array_index.h"
#include "mzpeak/data/signals.h"
#include "mzpeak/schema/psi/chunk_encoding.h"
#include "mzpeak/util/algorithm.h"
#include "mzpeak/util/numpress.h"
#include "mzpeak/util/slice.h"
#include "mzpeak/util/types.h"

namespace MzPeak::Data::Transformer::Primary {

/**
 * Transform Arrow arrays that have been encoded with one of the chunk
 * encoding schemes on the primary axis.
 */
template <Util::supported_type T> class Decoder final {
public:
  /// The type of result this function object returns.
  using result_type = std::variant<std::shared_ptr<arrow::Array>,
                                   std::pair<T, std::shared_ptr<arrow::Array>>,
                                   std::shared_ptr<std::vector<T>>>;

  /// Constructor.
  Decoder(std::shared_ptr<Signals>,
          std::shared_ptr<Util::Slice>,
          const ArrayIndex::Dimension&);

  /**
   * Transform the given array.
   *
   * This array could come from the `chunk_values` column, or from the
   * `chunk_transform` column.
   */
  result_type operator()(int64_t, const std::shared_ptr<arrow::Array>&) const;

private:
  std::string dim_name_;
  std::vector<Schema::PSI::ChunkEncoding> chunk_encoding_;
  std::vector<T> chunk_start_;
};

/******************************************************************************/
template <Util::supported_type T>
Decoder<T>::Decoder(std::shared_ptr<Signals> signals,
                    std::shared_ptr<Util::Slice> slice,
                    const ArrayIndex::Dimension& dim)
    : dim_name_(dim.name)
    , chunk_encoding_()
    , chunk_start_()
{
  auto decode = [&]<typename U>(Schema::BufferFormat format,
                                std::vector<U>& dest) -> void {
    std::optional<Schema::Column> column = signals->column(dim, format);

    if (!column.has_value()) {
      std::string msg("while decoding " + dim.name);
      msg += " a needed column with buffer format ";
      msg += Schema::buffer_format_to_string(format);
      msg += " was not found";
      throw InvalidFormatError(msg);
    }

    slice->array(*column, dest, Util::Decoders::Scalar<U>());
  };

  // Decode the `chunk_encoding` column.
  std::vector<std::string_view> encodings;
  decode(Schema::BufferFormat::ChunkEncoding, encodings);
  chunk_encoding_.reserve(encodings.size());

  for (const auto& s : encodings) {
    std::optional<Schema::CV> cv = Schema::CV::from_string(s);

    if (!cv.has_value()) {
      throw InvalidFormatError("invalid chunk encoding CV: " + std::string(s));
    }

    chunk_encoding_.emplace_back(*cv);
  }

  // Decode the `chunk_start` column.
  decode(Schema::BufferFormat::ChunkStart, chunk_start_);

  // Sanity checks:
  if (chunk_encoding_.size() != chunk_start_.size()) {
    std::string msg("while decoding " + dim.name);
    msg += " the chunk encoding and chunk start columns";
    msg += " have different lengths";
    throw InvalidFormatError(msg);
  }
}

/******************************************************************************/
template <Util::supported_type T>
Decoder<T>::result_type
Decoder<T>::operator()(int64_t index, const std::shared_ptr<arrow::Array>& src) const
{
  if (index < 0 || static_cast<std::size_t>(index) >= chunk_encoding_.size()) {
    std::string msg("while decoding " + dim_name_);
    msg += " the chunk_values/chunk_transform column is out of bounds ";
    msg += std::to_string(index) + " >= " + std::to_string(chunk_encoding_.size());
    throw InvalidFormatError(msg);
  }

  std::optional<Schema::PSI::ChunkEncoding::Type> type =
      chunk_encoding_[index].type();

  if (!type.has_value()) {
    std::string msg("while decoding " + dim_name_);
    msg += " unknown chunk encoding method: ";
    msg += chunk_encoding_[index].to_cv().to_string();
    throw InvalidFormatError(msg);
  }

  switch (type.value()) {
  case Schema::PSI::ChunkEncoding::Type::NoCompression:
    return std::make_pair(chunk_start_[index], src);
  case Schema::PSI::ChunkEncoding::Type::Delta:
    return Util::Algorithm::null_delta_decode<Util::enum_type_v<T>>(
        chunk_start_[index], src);
  case Schema::PSI::ChunkEncoding::Type::NumpressLinear:
    return Util::Numpress::decode_linear_convert<T>(src);
  }

  std::unreachable();
}

} // namespace MzPeak::Data::Transformer::Primary
