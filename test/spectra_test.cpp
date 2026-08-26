/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE Spectra
#include <boost/test/included/unit_test.hpp>

#include <ranges>

#include "mzpeak/open.h"
#include "mzpeak/spectra.h"
#include "mzpeak/spectrum.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_read_spectra)
{
  using namespace MzPeak;

  auto go = [](const std::string file_name) {
    auto mzpeak = MzPeak::open("../test/files/" + file_name);
    auto spectra = mzpeak.spectra();

    BOOST_TEST_CONTEXT("while using the " << file_name << "file")
    {
      BOOST_TEST((spectra.size() == 48));

      auto spectrum = spectra[0];
      auto mz = spectrum.mz();

      auto tolerance = boost::test_tools::tolerance(0.001);

      if (file_name == "small.numpress.mzpeak") {
        // Some numpress linear values are less precise than their
        // matching point or delta values.
        tolerance = boost::test_tools::tolerance(0.1);
      }

      BOOST_TEST(mz.size() == 13589);
      BOOST_TEST(mz[0] == 202.607, tolerance);
      BOOST_TEST(mz[mz.size() - 1] == 1999.840, tolerance);

      // Test some NULL values.
      BOOST_TEST_REQUIRE(mz[7] == 202.60831, tolerance);
      BOOST_TEST_REQUIRE(mz[8] == 202.60856, tolerance);
      BOOST_TEST_REQUIRE(mz[14] == 204.761, tolerance);
      BOOST_TEST_REQUIRE(mz[15] == 204.762, tolerance);

      // The m/z values should be monotonically increasing.
      for (std::size_t i : std::views::iota(1ul, mz.size())) {
        BOOST_TEST_REQUIRE(mz[i] > mz[i - 1]);
      }

      auto intensity = spectrum.intensity();
      BOOST_TEST_REQUIRE((intensity.size() == mz.size()));
      BOOST_TEST_REQUIRE(intensity[0] == 0.0, tolerance);
      BOOST_TEST_REQUIRE(intensity[1] == 1938.12, tolerance);
      BOOST_TEST_REQUIRE(intensity[7] == 0.0, tolerance);
      BOOST_TEST_REQUIRE(intensity[8] == 0.0, tolerance);
      BOOST_TEST_REQUIRE(intensity[9] == 1422.17, tolerance);
      BOOST_TEST_REQUIRE(intensity[intensity.size() - 1] == 0.0, tolerance);

      BOOST_TEST_REQUIRE(spectrum.ms_level() == 1u);
      BOOST_TEST_REQUIRE(spectrum.scan_time() == 0.004935, tolerance);
    }
  };

  go("small.mzpeak");
  go("small.chunked.mzpeak");
  go("small.numpress.mzpeak");
}
