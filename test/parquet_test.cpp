/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE Parquet
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/open.h"
#include "mzpeak/util/manager.h" // IWYU pragma: keep
#include "mzpeak/util/parquet.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_get_kv_string)
{
  using namespace MzPeak;

  auto mzpeak = MzPeak::open("../test/files/small.mzpeak");

  auto entry = mzpeak.find_file(Schema::EntityType::Type::Spectrum,
                                Schema::DataKind::DataArray);

  BOOST_TEST((entry != mzpeak.files().end()));

  auto parquet = mzpeak.manager()->parquet(*entry);
  auto fmd = parquet->file_metadata();
  auto et = parquet->index_file().entity_type();
  auto key = et.array_index_name();
  auto json = parquet->kv_string(fmd, key);

  BOOST_TEST(json.has_value());
}
