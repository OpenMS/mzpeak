/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include "mzpeak/exception.h"
#include "mzpeak/metadata/spectra.h"
#include "mzpeak/metadata/table.h"
#include "mzpeak/schema/group.h"
#include "mzpeak/util/algorithm.h"
#include "mzpeak/util/batch.h"
#include "mzpeak/util/filter.h"
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
  void load(std::size_t index);

  std::shared_ptr<Util::Manager> manager_;
  std::vector<Spectra::Metadata> metadata_;
  std::size_t begin_{};
  std::size_t end_{};
};

/******************************************************************************/
void Spectra::Impl::load(std::size_t index)
{
  if (index >= begin_ && index < end_) {
    // Already loaded.
    return;
  }

  auto it = manager_->find_file(Schema::EntityType::Type::Spectrum,
                                Schema::DataKind::Type::Metadata);

  if (it == manager_->files().end()) {
    throw InvalidFormatError("missing spectra metadata file");
  }

  std::shared_ptr<Util::Parquet> parquet = manager_->parquet(*it);

  Table table(parquet);
  std::shared_ptr<Schema::Group> group = table.group("root");

  Projection projection;
  auto index_field = projection.project(group, "index");
  auto level_field = projection.project(group, Group::CVType("MS", "1000511"));
  auto scan_time_field = projection.project(group, "time");
  auto delta_field = projection.project(group, "mz_delta_model");

  auto to_request = Util::Algorithm::range_to_request(
      manager_->metadata_cache_size(), sizeof(Spectra::Metadata), index);

  metadata_.clear();
  begin_ = to_request.first;
  end_ = to_request.second;

  Filter filter(Filter::ge(index_field.value(), static_cast<uint64_t>(begin_)));
  filter.and_(Filter::lt(index_field.value(), static_cast<uint64_t>(end_)));

  parquet->read(projection, filter, [&](const Batch& batch) -> bool {
    for (int64_t row : std::views::iota(0, batch.size())) {
      Spectra::Metadata md{.ms_level = batch.scalar<uint8_t>(row, level_field),
                           .scan_time = batch.scalar<double>(row, scan_time_field),
                           .delta_model = batch.scalar<double, std::vector<double>>(
                               row, delta_field)};

      metadata_.push_back(md);
    }

    return true;
  });
}

/******************************************************************************/
Spectra::Spectra(std::shared_ptr<Util::Manager> manager)
    : impl_(std::make_unique<Impl>(std::move(manager)))
{
}

/******************************************************************************/
Spectra::~Spectra() = default;

/******************************************************************************/
const Spectra::Metadata& Spectra::get(std::size_t index)
{
  impl_->load(index);

  std::size_t true_index = index - impl_->begin_;

  if (true_index >= impl_->metadata_.size()) {
    throw InvalidIteratorError("metadata request is out of bounds");
  }

  return impl_->metadata_[true_index];
}

} // namespace MzPeak::Metadata
