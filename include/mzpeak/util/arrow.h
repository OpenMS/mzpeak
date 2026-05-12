/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <arrow/io/api.h>

#include "mzpeak/file.h"

namespace MzPeak::Util {

/**
 * This is a low-level interface for accessing a Parquet file.
 */
class Arrow final {
public:
  using random_access_t = arrow::io::RandomAccessFile;

  /// Constructor.
  Arrow(std::unique_ptr<File::Readable>);

  /// Destructor.
  ~Arrow();

  /**
   * Return an arrow I/O object that can be used to open a Parquet
   * file for reading.
   */
  std::shared_ptr<random_access_t> reader() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Util
