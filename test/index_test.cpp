/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE Index
#include <boost/test/included/unit_test.hpp>

#include <ranges>

#include "mzpeak/index.h"
#include "mzpeak/open.h"
#include "mzpeak/schema/file.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_parse_json)
{
  auto index = MzPeak::open("../test/files/small.mzpeak");
  const auto& files = index.files();
  BOOST_TEST(!files.empty(), "files should not be empty but is");
}

/******************************************************************************/
BOOST_AUTO_TEST_CASE(is_associated_with)
{
  auto index = MzPeak::open("../test/files/small.mzpeak");

  const auto& files = index.files();
  const auto& spectra = index.find("spectra_data.parquet");
  BOOST_TEST((spectra != files.end()), "missing spectra_data.parquet");

  auto matches = [&](const auto& other) -> bool {
    return other != *spectra && spectra->is_associated_with(other);
  };

  for (const auto& other : files | std::views::filter(matches)) {
    BOOST_TEST_CONTEXT(spectra->file_name << " should not be associated with "
                                          << other.file_name)
    {
      BOOST_TEST((other.file_name == "spectra_metadata.parquet"));
    }
  }
}
