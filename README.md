<p align="center">
  <a href="https://github.com/OpenMS/mzpeak/search?l=c%2B%2B">
    <img alt="Language" src="https://img.shields.io/badge/lang-c%2B%2B23-red?style=for-the-badge">
  </a>

  <a href="https://github.com/OpenMS/mzpeak/actions">
    <img alt="CI" src="https://img.shields.io/github/actions/workflow/status/openms/mzpeak/test.yml?branch=trunk&style=for-the-badge">
  </a>

  <a href="https://github.com/OpenMS/mzpeak/blob/trunk/LICENSE">
    <img alt="License" src="https://img.shields.io/github/license/openms/mzpeak?style=for-the-badge&color=blue">
  </a>

  <a href="https://discord.com/channels/832282841836159006/1483925549179994332">
    <img alt="Discord" src="https://img.shields.io/discord/832282841836159006?style=for-the-badge&label=support&color=orange">
  </a>
</p>

# mzPeak

A C++ library that implements the mzPeak file format for efficiently
storing data acquired by a mass spectrometer.

mzPeak is a forthcoming standard format from HUPO-PSI.  For more
information please see:

  - https://github.com/HUPO-PSI/mzPeak

  - https://pubs.acs.org/doi/10.1021/acs.jproteome.5c00435

## About

The mzPeak C++ library provides both high- and low-level interfaces.

Most users will appreciate the high-level interface that abstracts
away most of the underlying format details.  This allows quick and
easy access to the stored data without sacrificing efficiency.

Those who want to read or write proprietary or encrypted tables can
use the low-level interface.  If necessary, direct access to the
Parquet reader and writer objects is provided.

**NOTE**: This is a *work in progress*, no stability is guaranteed at
this point.  Our current goal is to stabilize the API by the end of
the summer (2026).

## Features

- [X] Supports Linux, macOS, and Windows
- [X] Memory efficient, random access to stored data
- [X] Read local mzPeak files (zip archives or directories)
- [ ] Transparently read mzPeak files from the cloud (summer 2026)
- [X] Automatic detection and decoding of data tables
- [ ] Streaming writer interface (summer 2026)

## Example Spectra Reader

The following example, taken from the `examples/read_spectra.cpp`
file, reports some m/z values from the first few spectra in an mzPeak
file:

```c++
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
    std::print("{:9.2f} | ", spectrum->mz().front());
    std::print("{:8.2f} | ", spectrum->mz().back());
    std::println();
  }

  return 0;
}
```

Running the above with the `test/files/small.mzpeak` produces the
following output:

```
There are 48 spectra in this file.
Reviewing the first 5 spectra.

| Index | First m/z | Last m/z |
|-------|-----------|----------|
|     0 |    202.61 |  1999.84 |
|     1 |    200.09 |  1347.76 |
|     2 |    231.39 |  1911.64 |
|     3 |    236.05 |   368.35 |
|     4 |    203.22 |   396.97 |
```
