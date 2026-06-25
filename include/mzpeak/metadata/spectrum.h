/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <memory>
#include <optional>
#include <vector>

namespace MzPeak::Util {
class Slice;
}

namespace MzPeak::Schema {
class Struct;
}

namespace MzPeak::Metadata {

class Table;

/**
 * Metadata from the spectrum table.
 */
class Spectrum final {
public:
  /// Constructor.
  Spectrum(std::shared_ptr<Table>, uint64_t);

  /// Destructor.
  ~Spectrum() = default;

  /**
   * Return the spectrum level (MS:1000511).
   *
   * NOTE: If no level was found in the metadata this function will
   * return `nullopt`.
   */
  std::optional<uint8_t> ms_level() const { return ms_level_; }

  /**
   * Return the delta model need for decoding null marking in the
   * signals file.
   */
  const std::vector<double>& delta_model() const { return delta_model_; }

private:
  std::shared_ptr<Table> table_;
  std::shared_ptr<Schema::Struct> group_;
  uint64_t index_;

  std::optional<uint8_t> ms_level_;
  std::vector<double> delta_model_;
};

} // namespace MzPeak::Metadata
