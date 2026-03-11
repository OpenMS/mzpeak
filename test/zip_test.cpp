/*

This file is part of the package mzpeak.  It is subject to the license
in the LICENSE file found in the top-level directory of this project.

*/

#define BOOST_TEST_MODULE Zip
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/zip.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_list_files) {
  namespace fs = std::filesystem;

  MzPeak::Archive::Zip zip("../test/files/small.mzpeak");
  std::vector<fs::path> files(zip.list());

  bool expect = std::ranges::find(files, "mzpeak_index.json") != files.end();

  std::string paths;
  for (auto& i : files)
    paths += i.string() + ", ";

  BOOST_TEST(expect, paths << " is missing expected value");
}

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_read_file) {
  MzPeak::Archive::Zip zip("../test/files/small.mzpeak");
  std::unique_ptr<MzPeak::File::Readable> file(zip.read_file("mzpeak_index.json"));
  std::unique_ptr<std::istream> stream(MzPeak::File::to_istream(std::move(file)));
  std::string line;

  std::getline(*stream, line);
  BOOST_TEST(line == "{");
}

/******************************************************************************/
// Test seeking by reading the Parquet magic bytes in the footer.
BOOST_AUTO_TEST_CASE(can_seek_file) {
  MzPeak::Archive::Zip zip("../test/files/small.mzpeak");
  std::string magic("PAR1");

  std::unique_ptr<MzPeak::File::Readable> file(
      zip.read_file("spectra_data.parquet"));

  file->seek(file->size() - magic.size());

  std::optional<MzPeak::File::Readable::buffer_t> buffer_opt(
      file->read(magic.size()));
  BOOST_TEST(buffer_opt.has_value(), "buffer should be allocated");

  MzPeak::File::Readable::buffer_t buffer = buffer_opt.value();
  BOOST_TEST(buffer->size() == magic.size());

  std::string bytes(reinterpret_cast<char*>(buffer->data()), buffer->size());
  BOOST_TEST(bytes == magic);
}
