/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/exception.h"
#include "mzpeak/metadata/spectra.h"
#include "mzpeak/metadata/table.h"
#include "mzpeak/schema/group.h"
#include "mzpeak/util/batch.h"
#include "mzpeak/util/manager.h"
#include "mzpeak/util/projection.h"

namespace MzPeak::Metadata {

using namespace MzPeak::Util;
using namespace MzPeak::Schema;

/******************************************************************************/
struct Spectra::Impl {

  /// Constructor
  Impl(std::shared_ptr<Util::Manager> parquet)
      : manager_(std::move(parquet))
  {
  }

  /// Get the metadata from parquet.
  void load();

  std::shared_ptr<Util::Manager> manager_;
  std::vector<Spectra::Metadata> metadata_;
};

/******************************************************************************/
void Spectra::Impl::load()
{
  auto it = manager_->find_file(Schema::EntityType::Type::Spectrum,
                                Schema::DataKind::Type::Metadata);

  if (it == manager_->files().end()) {
    throw InvalidFormatError("missing spectra metadata file");
  }

  std::shared_ptr<Util::Parquet> parquet = manager_->parquet(*it);
  Table table(parquet);
  std::shared_ptr<Schema::Group> group = table.group("root");

  Projection projection;
  auto level_field = projection.project(group, Group::CVType("MS", "1000511"));
  auto scan_time_field = projection.project(group, "time");
  auto delta_field = projection.project(group, "mz_delta_model");

  auto read_batch = [&](const Batch& batch) -> bool {
    for (int64_t row : std::views::iota(0, batch.size())) {
      Spectra::Metadata md{.ms_level = batch.scalar<uint8_t>(row, level_field),
                           .scan_time = batch.scalar<double>(row, scan_time_field),
                           .delta_model = batch.scalar<double, std::vector<double>>(
                               row, delta_field)};

      metadata_.push_back(md);
    }

    return true;
  };

  parquet->read(read_batch, projection);
}

/******************************************************************************/
Spectra::Spectra(std::shared_ptr<Util::Manager> manager)
    : impl_(std::make_unique<Impl>(std::move(manager)))
{
  impl_->load();
}

/******************************************************************************/
Spectra::~Spectra() = default;

/******************************************************************************/
const Spectra::Metadata& Spectra::get(std::size_t index)
{
  return impl_->metadata_[index];
}

} // namespace MzPeak::Metadata
