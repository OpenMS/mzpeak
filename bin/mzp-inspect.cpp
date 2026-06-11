/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include "arrow/util/key_value_metadata.h"
#include "mzpeak/util/struct.h"
#include <boost/program_options.hpp>
#include <iostream>
#include <memory>
#include <mzpeak.h>
#include <print>

/******************************************************************************/
namespace po = boost::program_options;

/******************************************************************************/
std::unique_ptr<MzPeak::Util::Parquet> open_parquet_file(MzPeak::Index& index,
                                                         const std::string& file)
{
  auto it = std::ranges::find(index.files(), file, &MzPeak::Schema::File::file_name);

  if (it == index.files().end()) {
    std::println(stderr, "file \"{}\" is not in the mzPeak file index", file);
    return nullptr;
  }

  return index.parquet(*it);
}

/******************************************************************************/
int print_array_index(MzPeak::Index& index, const std::string& file)
{
  auto parquet = open_parquet_file(index, file);
  if (parquet == nullptr) return 1;

  std::print("{}", parquet->array_index_json());
  return 0;
}

/******************************************************************************/
int print_schema(MzPeak::Index& index, const std::string& file)
{
  auto parquet = open_parquet_file(index, file);
  if (parquet == nullptr) return 1;

  auto schema = parquet->file_metadata()->schema();
  std::println("{}", schema->ToString());
  return 0;
}

/******************************************************************************/
int print_structs(MzPeak::Index& index, const std::string& file)
{
  auto parquet = open_parquet_file(index, file);
  if (parquet == nullptr) return 1;

  auto structs =
      parquet->structs() | std::views::values | std::ranges::to<std::vector>();
  std::ranges::sort(structs, {}, &MzPeak::Util::Struct::index);

  for (const auto& s : structs) {
    std::println("{} [index:{}, fields:{}]", s->name(), s->index(),
                 s->fields().size());

    auto fields = s->fields() | std::views::values | std::ranges::to<std::vector>();
    std::ranges::sort(fields, {}, &MzPeak::Util::Struct::Field::index);

    for (const auto& field : fields) {
      std::string kind("?");
      std::string type("?");

      using enum MzPeak::Util::Struct::Field::Kind;
      switch (field->kind()) {
      case Scalar:
        kind = "scalar";
        break;
      case List:
        kind = "list";
        break;
      case Params:
        kind = "params";
        break;
      case Unknown:
        kind = "unknown";
        break;
      }

      if (field->data_type().has_value()) {
        type = MzPeak::Schema::PSI::data_type_to_string(field->data_type().value());
      }

      std::println("  | {} [index:{}, kind: {}, type:{}]", field->name(),
                   field->index(), kind, type);
    }
  }

  return 0;
}

/******************************************************************************/
int print_fmd_kv(MzPeak::Index& index,
                 const std::string& file,
                 std::optional<std::string> key)
{
  auto parquet = open_parquet_file(index, file);
  if (parquet == nullptr) return 1;

  const std::shared_ptr<const arrow::KeyValueMetadata>& kv =
      parquet->file_metadata()->key_value_metadata();

  if (key.has_value()) {
    auto res = kv->Get(*key);

    if (!res.ok()) {
      std::println(stderr, "no such key: {}", *key);
      return 1;
    }

    std::println("{}", res.ValueOrDie());
  } else {
    for (const std::string& key : kv->keys()) {
      std::println("{}", key);
    }
  }

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

    desc.add_options()("schema", po::value<std::string>(), "Print schema details");

    desc.add_options()("structs", po::value<std::string>(),
                       "Print struct information");

    desc.add_options()("fmdkv", po::value<std::string>(),
                       "Dump the file meta data kv store");

    desc.add_options()("fmd-key", po::value<std::string>(),
                       "Used with --fmdkv to print the value of the given key");

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
    } else if (vmap.count("schema")) {
      return print_schema(index, vmap["schema"].as<std::string>());
    } else if (vmap.count("structs")) {
      return print_structs(index, vmap["structs"].as<std::string>());
    } else if (vmap.count("fmdkv")) {
      std::optional<std::string> key;

      if (vmap.count("fmd-key")) {
        key = vmap["fmd-key"].as<std::string>();
      }

      return print_fmd_kv(index, vmap["fmdkv"].as<std::string>(), key);
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
