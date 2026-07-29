/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#define BOOST_TEST_MODULE Algorithm
#include <boost/test/included/unit_test.hpp>

#include <arrow/array.h>
#include <arrow/builder.h>

#include "mzpeak/util/algorithm.h"

/******************************************************************************/
BOOST_AUTO_TEST_CASE(null_delta_decode_with_no_nulls)
{
  using namespace MzPeak::Util;

  arrow::Status status;
  arrow::Int64Builder builder;
  int64_t vals[] = {1, 2, 3};
  int64_t start = 2;

  status = builder.AppendValues(vals, sizeof(vals) / sizeof(int64_t));
  BOOST_TEST(status.ok());

  std::shared_ptr<arrow::Array> input;
  status = builder.Finish(&input);
  BOOST_TEST(status.ok());

  std::shared_ptr<arrow::Array> output =
      Algorithm::null_delta_decode<Type::Int64>(start, input);

  std::shared_ptr<arrow::Int64Array> casted =
      std::static_pointer_cast<arrow::Int64Array>(output);

  std::vector<int64_t> decoded;
  decoded.reserve(casted->length());

  for (int64_t index : std::views::iota(0, casted->length())) {
    decoded.push_back(casted->Value(index));
  }

  BOOST_TEST(decoded.size() == casted->length());

  std::vector<int64_t> expected = {2, 3, 5, 8};
  BOOST_TEST(decoded == expected);
}
