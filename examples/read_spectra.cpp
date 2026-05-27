/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <mzpeak.h>
#include <print>
#include <ranges>

int main(int argc, char* argv[])
{
  if (argc < 2) {
    std::println(stderr, "Usage: {} file", std::string_view{argv[0]});
    return 1;
  }

  MzPeak::Index index = MzPeak::open(argv[1]);
  MzPeak::Spectra spectra = index.spectra();

  std::size_t to_review = std::min(5ul, spectra.size());

  std::println("There are {} spectra in this file.", spectra.size());
  std::println("Reviewing the first {} spectra.", to_review);

  auto enumerated_spectra =
      spectra | std::views::take(to_review) | std::views::enumerate;

  std::println();
  std::println("| Index | First m/z | Last m/z |");
  std::println("|-------|-----------|----------|");

  for (const auto& [index, spectrum] : enumerated_spectra) {
    std::print("| {:5d} | ", index);
    std::print("{:9.2f} | ", spectrum.mz().front());
    std::print("{:8.2f} | ", spectrum.mz().back());
    std::println();
  }

  return 0;
}
