/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <memory>
#include <optional>
#include <vector>

/******************************************************************************/
// Forward declarations.
namespace MzPeak::Util {
class Parquet;
class Manager;
} // namespace MzPeak::Util

/******************************************************************************/
namespace MzPeak::Metadata {

/**
 * Spectra metadata.
 */
class Spectra final {
public:
  /// The available metadata.
  struct Metadata {
    std::optional<uint8_t> ms_level;
    std::optional<double> scan_time;
    std::vector<double> delta_model;
  };

  /// Constructor.
  explicit Spectra(std::shared_ptr<Util::Manager>);

  /// Destructor.
  ~Spectra();

  /**
   * Get the metadata for the spectrum with the given ID.
   */
  const Metadata& get(std::size_t);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Metadata
