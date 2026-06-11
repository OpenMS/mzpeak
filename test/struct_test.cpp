/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#define BOOST_TEST_MODULE Struct
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/open.h"
#include "mzpeak/util/parquet.h"
#include "mzpeak/util/struct.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_parse_column_names)
{
  using namespace MzPeak::Util;

  // All components.
  Struct::Field a("MS_1000528_lowest_observed_mz_unit_MS_1000040", 0);
  BOOST_TEST(a.index() == 0);
  BOOST_TEST(a.name() == "lowest_observed_mz");
  BOOST_TEST((a.cv_type().has_value() && a.cv_type().value() == "MS:1000528"));
  BOOST_TEST((a.cv_unit().has_value() && a.cv_unit().value() == "MS:1000040"));

  // No unit.
  Struct::Field b("MS_1000016_scan_start_time", 0);
  BOOST_TEST(b.name() == "scan_start_time");
  BOOST_TEST((b.cv_type().has_value() && b.cv_type().value() == "MS:1000016"));
  BOOST_TEST((!b.cv_unit().has_value()));

  // Nmae only.
  Struct::Field c("mz", 0);
  BOOST_TEST(c.name() == "mz");
  BOOST_TEST(!c.cv_type().has_value());
  BOOST_TEST(!c.cv_unit().has_value());
}

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_load_all_structs)
{
  using namespace MzPeak::Util;
  auto mzpeak = MzPeak::open("../test/files/small.mzpeak");

  auto entry = std::ranges::find(mzpeak.files(), "spectra_metadata.parquet",
                                 &MzPeak::Schema::File::file_name);

  BOOST_TEST((entry != mzpeak.files().end()));

  auto parquet = mzpeak.parquet(*entry);
  auto structs = parquet->structs();

  BOOST_TEST((structs.size() == 4));

  auto spectrum = structs["spectrum"];
  auto spectrum_index = spectrum->field("index");

  BOOST_TEST((spectrum_index.has_value()));
  BOOST_TEST((spectrum_index->get().data_type().has_value()));
  BOOST_TEST((spectrum_index->get().data_type().value() ==
              MzPeak::Schema::PSI::DataType::Int64));
}
