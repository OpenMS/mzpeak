/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include "mzpeak/schema/psi/array_type.h"
#include "mzpeak/schema/psi/data_type.h"

namespace MzPeak::Data {

/**
 * Describes a single dimension from the signals data file.
 */
struct Dimension {
  /// The name of this dimension (e.g., "mz", "intensity", etc.)
  std::string name;

  /// Data type used for decodeing.
  Schema::PSI::DataType data_type = Schema::PSI::DataType::Float64;

  /// The type of elements stored in this dimension.
  Schema::PSI::ArrayType array_type = Schema::PSI::ArrayType::NonStandard;
};

} // namespace MzPeak::Data
