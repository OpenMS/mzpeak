/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <stdexcept>

namespace MzPeak::Exception {

/**
 * Error thrown when a JSON file could not be parsed.
 */
class JsonError final : public std::runtime_error {
public:
  /// Constructor.
  JsonError(const std::string& msg) : std::runtime_error(msg) {};

  /// Destructor.
  ~JsonError() = default;
};

} // namespace MzPeak::Exception
