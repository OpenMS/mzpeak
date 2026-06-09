/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <memory>

#include "mzpeak/util/parquet.h"

namespace MzPeak::Data {

/**
 * Low-level access to metadata files in a mzpeak file.
 */
class Metadata final {
public:
  /// Constructor.
  Metadata(std::unique_ptr<Util::Parquet>);

  /// Destructor.
  ~Metadata();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Data
