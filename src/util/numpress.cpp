/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <MSNumpress.hpp>

#include "mzpeak/exception.h"
#include "mzpeak/util/decoders.h"
#include "mzpeak/util/numpress.h"

namespace MzPeak::Util::Numpress {

/******************************************************************************/
void decode_linear(const std::vector<uint8_t>& input, std::vector<double>& output)
{
  try {
    ms::numpress::MSNumpress::decodeLinear(input, output);
  } catch (const char* msg) {
    throw InvalidFormatError(msg);
  }
}

/******************************************************************************/
std::shared_ptr<std::vector<double>>
decode_linear(const std::shared_ptr<arrow::Array>& src)
{
  if (src->type_id() != arrow::Type::UINT8) {
    std::string msg("numpress decoding requested but source array is not uint8");
    throw InvalidFormatError(msg);
  }

  std::vector<uint8_t> bytes;
  bytes.reserve(src->length());

  Decoders::Scalar<uint8_t> decoder;
  decoder.decode(src, bytes);

  auto values = std::make_shared<std::vector<double>>();
  decode_linear(bytes, *values);

  return values;
}

} // namespace MzPeak::Util::Numpress
