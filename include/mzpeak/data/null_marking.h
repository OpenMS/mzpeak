/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <memory>
#include <vector>

#include "mzpeak/util/algorithm.h"
#include "mzpeak/util/decoders.h"
#include "mzpeak/util/delta_estimator.h"

namespace MzPeak::Data::NullMarking {

using namespace MzPeak::Util;

/**
 * A class that can decode Null Marking (transform MS:1003902).
 */
template <typename T, typename U>
  requires convertible_between<T, U>
class Decoder final {
public:
  /// The type of arrow arrays we work with.
  using array_type = type_traits<enum_type_v<T>>::array_type;

  /// Constructor.
  explicit Decoder(const DeltaEstimator<U>& estimator);

  /**
   * Called by the decoder when a new array chuck is about to be
   * decoded.
   */
  void chunk(const std::shared_ptr<array_type>&);

  /**
   * Called by the decoder when a NULL value is encountered.
   */
  std::optional<T> operator()(int64_t);

private:
  // [begin, end)
  struct Range {
    int64_t begin;
    int64_t end;

    // The length of the range.
    int64_t size() const { return end - begin; }

    // Return `true` if the range is invalid.
    bool empty() const { return size() <= 0; }

    // Return the index of the closest non-null value.
    int64_t anchor(int64_t from) const { return (begin < from) ? (end - 1) : begin; }

    // Return the distance to the first non-null value.
    int64_t distance(int64_t from) const { return std::abs(from - anchor(from)); }
  };

  // The last null value that was decoded.
  struct Prior {
    int64_t index = -1;
    T value = {};
    T delta = {};
  };

  DeltaEstimator<U> estimator_;
  std::shared_ptr<array_type> array_;
  std::vector<Range> ranges_;
  std::vector<Range>::iterator next_range_;
  Prior prior_;
  T zero_ = {}; // When we need to return 0.
};

/******************************************************************************/
template <typename T, typename U>
  requires convertible_between<T, U>
Decoder<T, U>::Decoder(const DeltaEstimator<U>& estimator)
    : estimator_(estimator)
    , array_(nullptr)
    , ranges_()
    , next_range_(ranges_.end())
    , prior_()
{
}

/******************************************************************************/
/*
 * Build a "map" of the array we are about to decode.
 *
 * Specifically we need to know where all the non-NULL values are so
 * we can use them to estimate the NULL values.  When this function is
 * done the `ranges_` queue will contain all the `[begin, end)` ranges
 * of contiguous non-NULL values.
 */
template <typename T, typename U>
  requires convertible_between<T, U>
void Decoder<T, U>::chunk(const std::shared_ptr<array_type>& array)
{
  int64_t nulls = array->null_count();

  if (nulls <= 0) {
    // We won't be asked to decode anything.
    return;
  }

  array_ = array;
  prior_ = {};
  ranges_.clear();

  // The number of needed ranges will always be less than or equal to
  // the number of nulls.  Therefore we will sometimes reserve more
  // memory than we need, but it's still very small since
  // sizeof(Range) will be approximately 16 bytes.
  ranges_.reserve(nulls);

  Range range{0, 0};

  for (int64_t index : std::views::iota(0, array_->length())) {
    range.end = index;

    if (array_->IsNull(index)) {
      if (!range.empty()) ranges_.push_back(range);
      range.begin = index + 1;
    }
  }

  range.end = array_->length();
  if (!range.empty()) ranges_.push_back(range);
  next_range_ = ranges_.begin();
}

/******************************************************************************/
template <typename T, typename U>
  requires convertible_between<T, U>
std::optional<T> Decoder<T, U>::operator()(int64_t index)
{
  // Sanity check.
  if (array_ == nullptr) {
    throw("FIXME: assertion failed");
  };

  if (index != 0 && prior_.index == index - 1) {
    // We are in a run of NULL values and can use the estimated value
    // and delta from the last NULL to estimate the current NULL.
    prior_.index = index;
    prior_.value = prior_.value + prior_.delta;
    return prior_.value;
  } else if (!ranges_.empty() && next_range_ != ranges_.end()) {
    // We want the next range that ends on this null, or starts just
    // after this run of nulls.  This should be the range pointed to
    // by `next_range_` or the the range right after it.
    auto first =
        std::ranges::find_if(next_range_, ranges_.end(), [&index](const auto& r) {
          return r.end == index || r.begin > index;
        });

    // If we hit the end of the `ranges_` vector we will just use the
    // closest range which is `next_range_`.  This should never happen
    // in practice.
    if (first != ranges_.end()) {
      next_range_ = first;
    }

    Range range = *next_range_;
    prior_.index = index;

    if (range.size() == 1) {
      prior_.value =
          Decoders::unsafe_array_value<enum_type_v<T>>(array_, range.begin);
      prior_.delta = estimator_.predict(prior_.value);
    } else {
      auto slice = array_->Slice(range.begin, range.size());

      std::vector<T> values;
      values.reserve(slice->length());

      Decoders::Scalar<T> decoder;
      decoder.decode(slice, values);

      prior_.value =
          Decoders::unsafe_array_value<enum_type_v<T>>(array_, range.anchor(index));
      prior_.delta = Algorithm::median_delta(values, zero_);
    }

    // Delta may come from a range following a run of NULL values.
    T delta = prior_.delta * static_cast<T>(range.distance(index));

    if (range.begin < index) {
      prior_.value = prior_.value + delta;
    } else {
      prior_.value = prior_.value - delta;
    }

    return prior_.value;
  } else {
    // Shouldn't happen.
    throw("FIXME: failed to decode NULL marking value");
  }
}

} // namespace MzPeak::Data::NullMarking
