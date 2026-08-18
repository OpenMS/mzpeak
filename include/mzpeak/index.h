/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <memory>
#include <vector>

namespace MzPeak {

namespace IO {
class Archive;
}

namespace Schema {
class File;
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
   * Find a file in the mzPeak archive with the given name.
   */
  std::vector<Schema::File>::const_iterator find(std::string_view) const;

  /**
   * Access the spectra in the file.
   */
  Spectra spectra() const;

  /**
   * Access the low-level MzPeak Manager object.
   */
  std::shared_ptr<Util::Manager> manager() const;

protected:
  std::shared_ptr<Util::Manager> manager_;
};

} // namespace MzPeak
