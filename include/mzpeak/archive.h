/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <filesystem>
#include <vector>

#include "mzpeak/file.h"

namespace MzPeak::Archive {

namespace fs = std::filesystem;

/**
 * An interface for read-only archives.
 */
class Readable {
public:
  /// Destructor.
  virtual ~Readable() {};

  /**
   * Return a list of files names.
   */
  virtual std::vector<fs::path> list() = 0;

  /**
   * Open a file and gain random read-only access.
   */
  virtual std::unique_ptr<File::Readable> read_file(const fs::path&) = 0;

protected:
  Readable() = default;
};

} // namespace MzPeak::Archive
