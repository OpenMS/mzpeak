/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <filesystem>
#include <memory>
#include <stdexcept>

#include "mzpeak/directory.h"
#include "mzpeak/open.h"
#include "mzpeak/zip.h"

namespace MzPeak {

/******************************************************************************/
MzPeak::Index open(const fs::path& path)
{
  std::unique_ptr<MzPeak::Archive> archive;

  if (fs::exists(path)) {
    if (fs::is_directory(path)) {
      archive = std::make_unique<MzPeak::Directory>(path);
    } else {
      archive = std::make_unique<MzPeak::Zip>(path);
    }
  } else {
    // FIXME:
    throw std::runtime_error("network access not implemented");
  }

  return MzPeak::Index(std::move(archive));
}

} // namespace MzPeak
