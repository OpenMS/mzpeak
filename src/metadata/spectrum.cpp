/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <memory>

#include "mzpeak/exception.h"
#include "mzpeak/metadata/spectrum.h"
#include "mzpeak/metadata/table.h"
#include "mzpeak/schema/group.h"
#include "mzpeak/util/projection.h"
#include "mzpeak/util/slice.h"

namespace MzPeak::Metadata {

using namespace MzPeak::Schema;
using namespace MzPeak::Util;

/******************************************************************************/
Spectrum::Spectrum(std::unique_ptr<Util::Parquet> parquet, uint64_t index)
    : ms_level_()
    , delta_model_()
{
  Table table(std::move(parquet));
  std::shared_ptr<Schema::Group> group = table.group("root");

  Projection projection;
  auto level_field = projection.project(group, Group::CVType("MS", "1000511"));
  auto delta_field = projection.project(group, "mz_delta_model");

  std::unique_ptr<Slice> slice = table.indexed(index, group, projection);

  if (level_field.has_value()) {
    using ms_level_t = decltype(ms_level_)::value_type;
    using decoder = Decoders::Scalar<ms_level_t, ms_level_t>;
    slice->singleton<decoder>(*level_field, ms_level_);
  }

  if (delta_field.has_value()) {
    using delta_type = decltype(delta_model_)::value_type;
    using decoder = Decoders::List<delta_type>;
    slice->singleton<decoder, decltype(delta_model_)>(*delta_field, delta_model_);
  }
}

} // namespace MzPeak::Metadata
