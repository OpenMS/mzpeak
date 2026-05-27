/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include "mzpeak/index.h"
#include <filesystem>

namespace MzPeak {

namespace fs = std::filesystem;

/**
 * Open a MzPeak file for reading.
 */
MzPeak::Index open(const fs::path&);

}; // namespace MzPeak
