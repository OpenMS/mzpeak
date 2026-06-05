/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <boost/program_options.hpp>
#include <iostream>
#include <mzpeak.h>
#include <print>

/******************************************************************************/
namespace po = boost::program_options;

/******************************************************************************/
int print_array_index(MzPeak::Index& index, const std::string& file)
{
  auto it = std::ranges::find(index.files(), file, &MzPeak::Schema::File::file_name);

  if (it == index.files().end()) {
    std::println(stderr, "file \"{}\" is not in the mzPeak file index", file);
    return 1;
  }

  auto parquet = index.parquet(*it);
  std::print("{}", parquet->array_index_json());

  return 0;
}

/******************************************************************************/
int main(int argc, char* argv[])
{
  try {
    po::options_description desc("Usage: [options] file");

    desc.add_options()("help", "This message");
    desc.add_options()("file", po::value<std::string>(), "mzPeak file");

    desc.add_options()("array-index", po::value<std::string>(),
                       "Print array index for a Parquet file");

    po::positional_options_description pops;
    pops.add("file", 1);

    po::variables_map vmap;
    auto opts =
        po::command_line_parser(argc, argv).options(desc).positional(pops).run();
    po::store(opts, vmap);
    po::notify(vmap);

    if (vmap.count("help")) {
      desc.print(std::cout);
      return 0;
    }

    if (!vmap.count("file")) {
      std::println(stderr, "ERROR: missing mzPeak file");
      return 1;
    }

    MzPeak::Index index = MzPeak::open(vmap["file"].as<std::string>());

    if (vmap.count("array-index")) {
      return print_array_index(index, vmap["array-index"].as<std::string>());
    } else {
      std::println("WARN: no command given");
      return 1;
    }

  } catch (const std::exception& e) {
    std::println(stderr, "ERROR: {}", e.what());
    return 1;
  } catch (...) {
    std::println(stderr, "ERROR: unknown error thrown!");
    return 1;
  }

  return 0;
}
