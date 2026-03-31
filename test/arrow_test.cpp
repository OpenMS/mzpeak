/*

This file is part of the package mzpeak.  It is subject to the license
in the LICENSE file found in the top-level directory of this project.

*/

#define BOOST_TEST_MODULE Arrow
#include <arrow/io/api.h>
#include <boost/test/included/unit_test.hpp>
#include <parquet/arrow/reader.h>

#include "mzpeak/arrow.h"
#include "mzpeak/zip.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_open_parque) {
  MzPeak::Archive::Zip zip("../test/files/small.mzpeak");
  MzPeak::Arrow arrow(zip.read_file("chromatograms_data.parquet"));
  std::shared_ptr<arrow::io::RandomAccessFile> file(arrow.reader());

  arrow::Result<std::unique_ptr<parquet::arrow::FileReader>> arrow_reader(
      parquet::arrow::OpenFile(file, arrow::default_memory_pool()));

  BOOST_TEST(arrow_reader.ok(),
             "should have opened file: " << arrow_reader.status().ToString());
}
