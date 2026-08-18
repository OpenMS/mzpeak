/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <parquet/api/reader.h>

#include "mzpeak/exception.h"
#include "mzpeak/metadata/table.h"
#include "mzpeak/schema/group.h"
#include "mzpeak/util/parquet.h"

namespace MzPeak::Metadata {

/******************************************************************************/
struct Table::Impl {
  Impl(std::unique_ptr<Util::Parquet> parquet);
  ~Impl();

  std::unique_ptr<Util::Parquet> parquet_;
  std::optional<std::size_t> n_entries;
};

/******************************************************************************/
Table::Impl::Impl(std::unique_ptr<Util::Parquet> parquet)
    : parquet_(std::move(parquet))
    , n_entries()
{
  auto file = parquet_->index_file();

  if (file.data_kind() != Schema::DataKind::Metadata) {
    std::string msg("file is not a metadata file: " + file.file_name());
    throw InvalidFormatError(msg);
  }
}

/******************************************************************************/
Table::Impl::~Impl() = default;

/******************************************************************************/
Table::Table(std::unique_ptr<Util::Parquet> parquet)
    : impl_(std::make_unique<Impl>(std::move(parquet)))
{
}

/******************************************************************************/
Table::~Table() = default;

/******************************************************************************/
std::shared_ptr<Schema::Group> Table::group(std::string_view name) const
{
  const std::shared_ptr<Schema::GroupMap>& map = impl_->parquet_->groups();
  auto it = map->find(std::string(name));

  if (it == map->end()) {
    throw InvalidFormatError("schema is missing the " + std::string(name) +
                             " group");
  } else {
    return it->second;
  }
}

/******************************************************************************/
std::unique_ptr<Util::Slice>
Table::indexed(uint64_t index,
               const std::shared_ptr<Schema::Group>& group,
               const Util::Projection& projection) const
{
  auto index_field = group->field("index");
  if (!index_field.has_value()) return nullptr;

  auto column = std::make_pair(group, index_field.value());
  Util::Query q = Util::Query::Builder(column).eq(index);

  auto plan = impl_->parquet_->planner(q).plan();
  return impl_->parquet_->executor(projection).execute(plan);
}

} // namespace MzPeak::Metadata
