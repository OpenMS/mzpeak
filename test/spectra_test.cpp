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

  BOOST_TEST((mz.size() == 13589));
  BOOST_TEST(mz[0] == 202.607, boost::test_tools::tolerance(0.001));
  BOOST_TEST(mz[mz.size() - 1] == 1999.840, boost::test_tools::tolerance(0.001));
}
