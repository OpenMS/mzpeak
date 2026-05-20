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

## Example Reader

```c++
#include <iostream>
#include <mzpeak.h>

void main() {
  auto index = MzPeak::open("test/files/small.mzpeak");
  auto spectrum = index.spectra()[0];

  for (auto& mz : spectrum.mz()) {
    std::cout << mz << std::endl;
  }
}
```
