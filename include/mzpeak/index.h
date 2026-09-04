/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <memory>
#include <vector>

#include "mzpeak/schema/file.h"

namespace MzPeak {

namespace IO {
class Archive;
}

namespace Util {
class Manager;
}

class Spectra;

/**
 * Read-only access to the index inside a MzPeak archive.
 */
class Index {
public:
  /// Constructor.
  Index(std::unique_ptr<MzPeak::IO::Archive>);

  /**
   * Return a list of files found in the index.
   */
  const std::vector<Schema::File>& files() const;

  /**
   * Find a file given its `EntityType` and `DataKind`.
   */
  std::vector<Schema::File>::const_iterator find_file(Schema::EntityType::Type,
                                                      Schema::DataKind::Type) const;

  /**
   * Indicates which source to fetch spectra data from.
   */
  enum class SpectraSource {
    /// Use the `spectra_data.parquet` file which may contain profile
    /// or centroid spectra data.
    Data,

    /// Use the `spectra_peaks.parquet` file which is optional and may
    /// not exist.
    Peaks,
  };

  /**
   * Returns `true` if the give source file exists.
   */
  bool has_spectra(SpectraSource) const;

  /**
   * Access the spectra in the file.
   *
   * Throws an exception if `SpectraSource::Peaks` is request and does
   * not exist.  Use the `has_spectra` function to check for a peaks
   * source before call this function.
   */
  Spectra spectra(SpectraSource source = SpectraSource::Data) const;

  /**
   * Access the low-level MzPeak Manager object.
   */
  std::shared_ptr<Util::Manager> manager() const;

protected:
  std::shared_ptr<Util::Manager> manager_;
};

} // namespace MzPeak
