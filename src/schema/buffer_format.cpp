/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/schema/buffer_format.h"

namespace MzPeak::Schema {

/******************************************************************************/
std::string buffer_format_to_string(BufferFormat v) {
  using enum BufferFormat;

  switch (v) {
  case Point:
    return "point";
  case ChunkStart:
    return "chunk_start";
  case ChunkEnd:
    return "chunk_end";
  case ChunkValues:
    return "chunk_values";
  case ChunkEncoding:
    return "chunk_encoding";
  case ChunkSecondary:
    return "chunk_secondary";
  default:
    return "point";
  }
}

/******************************************************************************/
BufferFormat buffer_format_from_string(const std::string_view& s) {
  using enum BufferFormat;

  if (s == "point") {
    return Point;
  } else if (s == "chunk_start") {
    return ChunkStart;
  } else if (s == "chunk_end") {
    return ChunkEnd;
  } else if (s == "chunk_values") {
    return ChunkValues;
  } else if (s == "chunk_encoding") {
    return ChunkEncoding;
  } else if (s == "chunk_secondary") {
    return ChunkSecondary;
  } else {
    return Point;
  }
}

} // namespace MzPeak::Schema
