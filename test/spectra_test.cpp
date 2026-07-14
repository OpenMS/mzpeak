/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE Spectra
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/open.h"
#include "mzpeak/spectra.h"
#include "mzpeak/spectrum.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_read_spectra)
{
  using namespace MzPeak;

  auto mzpeak = MzPeak::open("../test/files/small.dir");
  auto spectra = mzpeak.spectra();

  BOOST_TEST((spectra.size() == 48));

  auto spectrum = spectra[0];
  auto mz = spectrum.mz();

  BOOST_TEST(mz.size() == 13589);
  BOOST_TEST(mz[0] == 202.607, boost::test_tools::tolerance(0.001));
  BOOST_TEST(mz[mz.size() - 1] == 1999.840, boost::test_tools::tolerance(0.001));

  // Test some NULL values.
  BOOST_TEST(mz[7] == 202.608, boost::test_tools::tolerance(0.001));
  BOOST_TEST(mz[8] == 202.609, boost::test_tools::tolerance(0.001));
  BOOST_TEST(mz[14] == 204.761, boost::test_tools::tolerance(0.001));
  BOOST_TEST(mz[15] == 204.762, boost::test_tools::tolerance(0.001));

  // The m/z values should be monotonically increasing.
  for (std::size_t i : std::views::iota(1ul, mz.size())) {
    BOOST_TEST(mz[i] > mz[i - 1]);
  }

  auto intensity = spectrum.intensity();
  BOOST_TEST((intensity.size() == mz.size()));
  BOOST_TEST(intensity[0] == 0.0, boost::test_tools::tolerance(0.001));
  BOOST_TEST(intensity[1] == 1938.12, boost::test_tools::tolerance(0.001));
  BOOST_TEST(intensity[intensity.size() - 1] == 0.0,
             boost::test_tools::tolerance(0.001));

  BOOST_TEST(spectrum.ms_level() == 1u);
}
