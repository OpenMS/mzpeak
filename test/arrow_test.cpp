/*

This file is part of the package mzpeak.  It is subject to the license
in the LICENSE file found in the top-level directory of this project.

*/

#define BOOST_TEST_MODULE Arrow
#include <boost/test/included/unit_test.hpp>

#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

#include "mzpeak/io/directory.h"
#include "mzpeak/util/arrow.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_open_parque)
{
  MzPeak::IO::Directory dir("../test/files/small.dir");
  MzPeak::Util::Arrow arrow(dir.read_file("spectra_data.parquet"));
  std::shared_ptr<arrow::io::RandomAccessFile> file(arrow.reader());

  arrow::Result<std::unique_ptr<parquet::arrow::FileReader>> arrow_reader(
      parquet::arrow::OpenFile(file, arrow::default_memory_pool()));

  BOOST_TEST_CONTEXT("should have opened file: " << arrow_reader.status().ToString())
  {
    BOOST_TEST(arrow_reader.ok());
  }

  auto reader = std::move(arrow_reader.ValueOrDie());
  arrow::Result result = reader->ReadTable();

  BOOST_TEST(result.ok(), result.status().ToString());
}
