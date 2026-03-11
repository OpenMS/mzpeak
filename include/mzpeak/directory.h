/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include "mzpeak/archive.h"

namespace MzPeak::Archive {

/**
 * Access files from a directory.
 */
class Directory final : public Readable {
public:
  /// Constructor.
  Directory(const fs::path& path) : path_(path.lexically_normal()) {}

  /**
   * Return a list of files in the directory.
   *
   * NOTE: This is not a recursive listing.
   *
   * NOTE: Only files will be returned.
   */
  std::vector<fs::path> list();

  /**
   * Open a file for reading.
   *
   * The path *must* be a file name relative to the directory as
   * returned by the `list` method.
   */
  std::unique_ptr<File::Readable> read_file(const fs::path& path);

private:
  fs::path path_;
};

} // namespace MzPeak::Archive
