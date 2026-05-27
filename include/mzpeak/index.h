/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include "mzpeak/archive.h"
#include "mzpeak/schema/file.h"
#include "mzpeak/spectra.h"
#include "mzpeak/util/parquet.h"

namespace MzPeak {

// Internal implementation.

/**
 * Read-only access to the index inside a MzPeak archive.
 */
class Index {
public:
  /// Constructor.
  Index(std::unique_ptr<MzPeak::Archive>);

  /// Destructor.
  ~Index();

  /**
   * Return a list of files found in the index.
   */
  const std::vector<Schema::File>& files() const;

  /**
   * Access the spectra in the file.
   */
  Spectra spectra() const;

  /**
   * Open a Parquet file directly.
   */
  std::unique_ptr<Util::Parquet> parquet(const Schema::File&) const;

protected:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak
