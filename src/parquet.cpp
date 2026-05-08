/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <arrow/util/key_value_metadata.h>

#include <functional>

#include "mzpeak/index/entity_type.h"
#include "mzpeak/parquet.h"
#include <memory>
#include <ranges>

namespace MzPeak {

/******************************************************************************/
struct Parquet::Impl {
  Impl(std::unique_ptr<parquet::ParquetFileReader> file) : file_(std::move(file)) {};
  ~Impl() = default;

  std::unique_ptr<parquet::ParquetFileReader> file_;
  std::optional<std::size_t> num_entities_;
};

/******************************************************************************/
Parquet::Parquet(std::unique_ptr<parquet::ParquetFileReader> file)
    : impl_(std::make_unique<Impl>(std::move(file))) {}

/******************************************************************************/
Parquet::~Parquet() = default;

/******************************************************************************/
template <typename T>
std::vector<T> Parquet::map_row_group_metadata(
    std::function<T(const parquet::RowGroupMetaData&)> func) {
  auto file_md(impl_->file_->metadata());
  int num(file_md->num_row_groups());

  if (num > 0) {
    std::vector<T> rs;
    rs.reserve(num);

    for (int i : std::ranges::views::iota(0, num)) {
      auto rg(file_md->RowGroup(i));
      rs.push_back(std::invoke(func, *rg));
    }

    return std::move(rs);
  } else {
    return {};
  }
}

/******************************************************************************/
std::optional<std::string> Parquet::metadata_kv(const std::string& key) const {
  auto file_md(impl_->file_->metadata());
  auto result(file_md->key_value_metadata()->Get(key));

  if (result.ok()) {
    return result.ValueOrDie();
  } else {
    return {};
  }
}

/******************************************************************************/
std::size_t Parquet::num_entities(Index::EntityType entity_type) {
  if (impl_->num_entities_.has_value()) {
    return impl_->num_entities_.value();
  } else {
    // Try the key-value metadata:
    {
      std::string key(Index::entity_type_to_string(entity_type) + "_count");
      auto value(metadata_kv(key));

      if (value.has_value()) {
        std::size_t pos;
        std::size_t num(std::stoull(*value, &pos));

        if (pos == value->size()) {
          impl_->num_entities_ = num;
          return num;
        }
      }
    }

    // Try the row group metadata:
    {
      // std::vector<int64_t> counts(
      //     map_row_group_metadata<int64_t>(&parquet::RowGroupMetaData::num_rows));
      //
      // if (!counts.empty()) {
      //   std::size_t max = std::ranges::max(counts);
      //   impl_->num_entities_ = max;
      //   return max;
      // }
    }

    // Oh well.
    return 0;
  }
}

} // namespace MzPeak
