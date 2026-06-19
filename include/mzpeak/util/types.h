/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include "mzpeak/util/struct.h"

namespace MzPeak::Util {

/**
 * Mapping from struct name to a Struct.
 */
using StructMap = std::map<std::string, std::shared_ptr<Struct>>;

/**
 * A parquet column can be uniquely identified using its parent struct
 * and field.
 */
using Column =
    std::pair<std::shared_ptr<const Struct>, std::shared_ptr<const Struct::Field>>;

} // namespace MzPeak::Util
