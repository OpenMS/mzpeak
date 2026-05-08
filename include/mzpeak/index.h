/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include "mzpeak/archive.h"
#include "mzpeak/index/file.h"

#include <boost/json.hpp>

namespace MzPeak::Index {

// Save some typing.
namespace json = boost::json;

// Internal implementation.
struct Impl;

/**
 * Read-only access to the index inside a MzPeak archive.
 */
class Readable {
public:
  /// Constructor.
  Readable(MzPeak::Archive::Readable&);

  /// Destructor.
  ~Readable();

  /**
   * Return a list of files found in the index.
   */
  const std::vector<Index::File>& files() const;

protected:
  std::unique_ptr<Impl> impl_;
};

} // namespace MzPeak::Index
