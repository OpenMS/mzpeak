/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE ArrayIndex
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/data/signals.h"
#include "mzpeak/open.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_get_array_index)
{
  using namespace MzPeak;

  auto mzpeak = MzPeak::open("../test/files/small.mzpeak");

  auto entry = std::ranges::find(mzpeak.files(), Schema::EntityType::Spectrum,
                                 &Schema::File::entity_type);

  BOOST_TEST((entry != mzpeak.files().end()));

  auto parquet = mzpeak.parquet(*entry);
  Data::Signals data(std::move(parquet));
  std::shared_ptr<Data::ArrayIndex> index(data.array_index());

  BOOST_TEST(index->prefix() == "point");
  BOOST_TEST(index->entries().size() == 2ul);

  std::optional<Schema::CV> transform_cv(Schema::CV::from_string("MS:1003901"));
  BOOST_TEST((transform_cv.has_value()));
  std::optional<Schema::PSI::Transform> transform(transform_cv.value());

  // NOTE: Due to a sort after parsing the index, the m/z array gets
  // moved to the end of the index.
  BOOST_TEST((index->entries()[1].array_name == "m/z array"));
  BOOST_TEST((index->entries()[1].buffer_format == Schema::BufferFormat::Point));
  BOOST_TEST((index->entries()[1].context == Schema::EntityType::Spectrum));
  BOOST_TEST((index->entries()[1].path == "point.mz"));
  BOOST_TEST((index->entries()[1].data_type == Schema::PSI::DataType::Float64));
  BOOST_TEST((index->entries()[1].array_type == Schema::PSI::ArrayType::Mz));
  BOOST_TEST((index->entries()[1].unit == "MS:1000040"));
  BOOST_TEST((index->entries()[1].buffer_priority));
  BOOST_TEST((index->entries()[1].sorting_rank == std::optional{0}));
  BOOST_TEST((index->entries()[1].data_processing_id == std::nullopt));
  BOOST_TEST((index->entries()[1].transform == transform));

  std::size_t count = index->num_entities().value_or(0);
  BOOST_TEST(count == 48ul);
}

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_read_mz_array)
{
  using namespace MzPeak;

  auto mzpeak = MzPeak::open("../test/files/small.mzpeak");
  auto entry = std::ranges::find(mzpeak.files(), Schema::EntityType::Spectrum,
                                 &Schema::File::entity_type);

  BOOST_TEST((entry != mzpeak.files().end()));

  auto parquet = mzpeak.parquet(*entry);

  Data::Signals data(std::move(parquet));

  auto index_field = data.field("spectrum_index");
  auto mz_column = data.field("mz");

  BOOST_TEST(index_field.has_value());
  BOOST_TEST(mz_column.has_value());
}
