/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <memory>

#include "mzpeak/archive.h"
#include "mzpeak/index/file.h"

namespace MzPeak {

/**
 * Low-level access to metadata files in a mzpeak file.
 */
class Metadata final {
public:
  using readable_archive_t = std::shared_ptr<MzPeak::Archive::Readable>;

  /**
   * Constructor.
   *
   * NOTE: The given Index::File object *must* be a
   * Index::File::DataKind::Metadata.
   */
  Metadata(readable_archive_t archive, const Index::File&);

  /// Destructor.
  ~Metadata();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak
