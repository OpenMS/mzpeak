/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <arrow/io/api.h>

#include "mzpeak/file.h"

namespace MzPeak {

/**
 * This is a low-level interface for accessing a Parquet file.
 */
class Arrow final {
public:
  /// Constructor.
  Arrow(std::unique_ptr<File::Readable>);

  /**
   * Return an arrow I/O object that can be used to open a Parquet
   * file for reading.
   */
  std::shared_ptr<arrow::io::RandomAccessFile> reader();

private:
  std::shared_ptr<File::Readable> file_;
};

} // namespace MzPeak
