/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <memory>

#include "mzpeak/io/archive.h"
#include "mzpeak/schema/file.h"
#include "mzpeak/util/parquet.h"

namespace MzPeak::Util {

/**
 * FIXME: Write documentation!
 */
class Manager final {
public:
  /// Constructor.
  Manager(std::unique_ptr<MzPeak::IO::Archive>);

  /**
   * Return a vector of files that are located in the mzPeak archive.
   */
  const std::vector<Schema::File>& files() const;

  /**
   * Find a file given its name.
   */
  std::vector<Schema::File>::const_iterator
  find_file(const std::string_view& name) const;

  /**
   * Open a Parquet file from the mzPeak archive.
   */
  std::unique_ptr<Util::Parquet> parquet(const Schema::File&) const;

private:
  std::shared_ptr<MzPeak::IO::Archive> archive_;
  std::vector<Schema::File> files_;
};

} // namespace MzPeak::Util
