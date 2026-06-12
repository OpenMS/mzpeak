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

  auto mzpeak = MzPeak::open("../test/files/small.mzpeak");
  auto spectra = mzpeak.spectra();

  BOOST_TEST(spectra.size() == 48);

  auto spectrum = spectra[0];
  auto mz = spectrum.mz();
  BOOST_TEST(mz.size() == 13589);
  BOOST_TEST(mz[0] == 202.607, boost::test_tools::tolerance(0.001));
  BOOST_TEST(mz[mz.size() - 1] == 1999.840, boost::test_tools::tolerance(0.001));
}

/******************************************************************************/
// Regression: profile spectra whose first matching row is not at the start of
// its record batch were over-read (data_arrays.cpp slice length used an
// absolute end index instead of a row count).  Spectra 1 and 7 exercise this;
// spectrum 0 (which starts at row 0) always read correctly.
BOOST_AUTO_TEST_CASE(reads_profile_arrays_beyond_first_spectrum)
{
  auto mzpeak = MzPeak::open("../test/files/small.mzpeak");
  auto spectra = mzpeak.spectra();

  auto s1 = spectra[1].mz();
  BOOST_TEST(s1.size() == 18177);
  BOOST_TEST(s1.front() == 200.0909, boost::test_tools::tolerance(0.001));
  BOOST_TEST(s1.back() == 1999.8182, boost::test_tools::tolerance(0.001));

  auto s7 = spectra[7].mz();
  BOOST_TEST(s7.size() == 10329);
  BOOST_TEST(s7.back() == 1832.2386, boost::test_tools::tolerance(0.001));
}
