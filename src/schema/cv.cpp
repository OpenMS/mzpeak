/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include "mzpeak/schema/cv.h"

namespace MzPeak::Schema {

/******************************************************************************/
std::optional<CV> CV::from_string(const std::string_view& s)
{
  std::size_t sep_pos = s.find(':');

  if (sep_pos == std::string_view::npos) {
    return {};
  }

  return CV(std::string(s[0], sep_pos),
            std::string(s[sep_pos + 1], s.size() - (sep_pos + 1)));
}

/******************************************************************************/
std::string CV::to_string() const { return code_ + ":" + accession_; }

} // namespace MzPeak::Schema
