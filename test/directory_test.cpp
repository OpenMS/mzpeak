/*

This file is part of the package mzpeak.  It is subject to the license
in the LICENSE file found in the top-level directory of this project.

*/

#define BOOST_TEST_MODULE Directory
#include <boost/test/included/unit_test.hpp>

#include "mzpeak/directory.h"
#include "mzpeak/file.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_list_files)
{
  namespace fs = std::filesystem;

  MzPeak::Directory dir("../src");
  std::vector<fs::path> files(dir.list());

  bool expect = std::ranges::find(files, "directory.cpp") != files.end();

  std::string paths;
  for (auto& i : files)
    paths += i.string() + ", ";

  BOOST_TEST(expect, paths << " is missing expected value");
}

/******************************************************************************/
BOOST_AUTO_TEST_CASE(can_read_file)
{
  MzPeak::Directory dir("../src");
  std::unique_ptr<MzPeak::File> file(dir.read_file("directory.cpp"));
  std::unique_ptr<std::istream> stream(MzPeak::to_istream(std::move(file)));
  std::string line;

  std::getline(*stream, line);
  BOOST_TEST(line == "/*");
}
