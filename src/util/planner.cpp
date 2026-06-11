/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#include <arrow/array.h>
#include <arrow/record_batch.h>
#include <iterator>
#include <memory>
#include <parquet/api/reader.h>
#include <parquet/arrow/reader.h>
#include <parquet/page_index.h>
#include <parquet/statistics.h>
#include <print>
#include <ranges>

#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/util/parquet_types.h"
#include "mzpeak/util/planner.h"

namespace MzPeak::Util {

namespace psi = Schema::PSI;
using namespace std::placeholders;

/******************************************************************************/
/**
 * Used to return column statistics.
 *
 * The column metadata needs to outlive the stats, which is why both
 * are stored here.
 */
struct StatsCache {
  using key_type = int32_t;
  using value_type = std::pair<std::shared_ptr<parquet::ColumnChunkMetaData>,
                               std::shared_ptr<parquet::Statistics>>;

  // A statistics cache.  May contain null values which indicate that
  // a previous attempt failed and to not try again.  Also used to
  // know how many unique columns were requested.
  std::map<key_type, value_type> cache_;

  // Construct a key for the given struct/field.
  key_type key(const Query::destination_t& dest) const
  {
    return dest.second->absolute_index();
  }

  // Attempt to fetch column statistics.
  std::optional<value_type> get(const std::shared_ptr<parquet::RowGroupMetaData>& rg,
                                const Query::destination_t& dest)
  {
    key_type k(key(dest));
    auto it = cache_.find(k);

    if (it != cache_.end()) {
      value_type v = it->second;

      if (v.first == nullptr || v.second == nullptr) {
        return {};
      } else {
        return v;
      }
    } else {
      value_type v = load(rg, k).value_or({nullptr, nullptr});
      cache_[k] = v;

      if (v.first == nullptr || v.second == nullptr) {
        return {};
      } else {
        return v;
      }
    }
  }

  // Called on a cache miss to find the a column stats.
  std::optional<value_type>
  load(const std::shared_ptr<parquet::RowGroupMetaData>& rg, int32_t column_index)
  {
    std::shared_ptr<parquet::ColumnChunkMetaData> chunk =
        rg->ColumnChunk(column_index);
    if (!chunk->is_stats_set()) return {};

    std::shared_ptr<parquet::Statistics> stats(chunk->statistics());
    if (!stats) return {};

    return {std::make_pair(chunk, stats)};
  }
};

/******************************************************************************/
// Helper cache for the parquet page index.
class IndexCache {
public:
  // Cache items.
  struct Item {
    Item() {}

    Item(const std::shared_ptr<parquet::ColumnIndex>& c,
         const std::shared_ptr<parquet::OffsetIndex>& o)
        : page_count(o->page_locations().size())
        , col_index(c)
        , off_index(o)
    {
    }

    // Return true if this item is valid.
    bool valid() const
    {
      return page_count > 0 && col_index != nullptr && off_index != nullptr;
    }

    std::size_t page_count = 0;
    std::size_t current_page = 0;
    std::shared_ptr<parquet::ColumnIndex> col_index = nullptr;
    std::shared_ptr<parquet::OffsetIndex> off_index = nullptr;
  };

  // Store pointers so we can keep the items updated.
  using value_type = std::shared_ptr<Item>;

  // Keyed on the column path.
  using key_type = int32_t;

  /// Constructor.
  explicit IndexCache(const std::shared_ptr<parquet::RowGroupMetaData>& rg,
                      const std::shared_ptr<parquet::RowGroupPageIndexReader>& ri)
      : rg_(rg)
      , ri_(ri)
  {
  }

  // Construct a key for the given struct/field.
  //
  // FIXME: Should probably move this into the destination code.
  key_type key(const Query::destination_t& dest) const
  {
    return dest.second->absolute_index();
  }

  // Return an item from the cache, or compute it.
  std::optional<value_type> get(const Query::destination_t& dest)
  {
    key_type k(key(dest));
    auto it = cache_.find(k);

    if (it != cache_.end()) {
      if (it->second->valid()) {
        return it->second;
      } else {
        return {};
      }
    } else {
      value_type v = load(k).value_or(std::make_shared<Item>());
      cache_[k] = v;

      if (v->valid()) {
        return v;
      } else {
        return {};
      }
    }
  }

  // Return the page index with the maximum number of pages.
  std::optional<value_type> max_page_count() const
  {
    if (cache_.empty()) return {};
    value_type result = nullptr;

    for (const auto& kv : cache_) {
      if (!kv.second->valid()) return {};

      if (result == nullptr || kv.second->page_count > result->page_count) {
        result = kv.second;
      }
    }

    if (result != nullptr) {
      return result;
    } else {
      return {};
    }
  }

  // Generate ranges and insert them into the given vector.
  void generate_ranges(const std::vector<bool>& pages,
                       std::vector<Planner::Range>& ranges,
                       int32_t row_group,
                       value_type major_index)
  {
    auto i = std::ranges::find(pages.begin(), pages.end(), true);
    auto j = pages.end();

    for (; i != pages.end(); i = std::ranges::find(j, pages.end(), true)) {
      j = std::ranges::find(i + 1, pages.end(), false);

      std::size_t first_page =
          static_cast<std::size_t>(std::distance(pages.begin(), i));

      std::size_t last_page =
          static_cast<int64_t>(std::distance(pages.begin(), j - 1));

      auto page_map = major_index->off_index->page_locations();

      int64_t first_row_index = page_map[first_page].first_row_index;

      // The index of the last row is computed by using the first row
      // of the next page.  But if we are already on the last page we
      // just use the total number of rows in the row group to get the
      // last row index.
      int64_t last_row_index = (last_page + 1 < major_index->page_count)
                                   ? page_map[last_page + 1].first_row_index - 1
                                   : rg_->num_rows() - 1;

      ranges.push_back(
          {row_group, first_row_index, last_row_index - first_row_index + 1});
    }
  }

private:
  bool has_page_index(key_type column_index)
  {
    auto check = [](const std::optional<parquet::IndexLocation>& loc) -> bool {
      return loc.has_value() && loc->offset >= 0 && loc->length > 0;
    };

    std::unique_ptr<parquet::ColumnChunkMetaData> ccmd =
        rg_->ColumnChunk(column_index);

    return check(ccmd->GetColumnIndexLocation()) &&
           check(ccmd->GetOffsetIndexLocation());
  }

  std::optional<value_type> load(key_type column_index)
  {
    if (!has_page_index(column_index)) return {};

    std::shared_ptr<parquet::ColumnIndex> cindex = ri_->GetColumnIndex(column_index);
    if (cindex == nullptr) return {};

    std::shared_ptr<parquet::OffsetIndex> oindex = ri_->GetOffsetIndex(column_index);
    if (oindex == nullptr) return {};

    return std::make_shared<Item>(cindex, oindex);
  }

  // Internal vars.
  std::shared_ptr<parquet::RowGroupMetaData> rg_;
  std::shared_ptr<parquet::RowGroupPageIndexReader> ri_;

  // The actual cache.
  std::map<key_type, value_type> cache_;
};

/******************************************************************************/
/**
 * Fetch the minimum and maximum values for a column using either
 * column statistics or the page index.
 */
struct ColMinMax final {
  // Use column statistics.
  ColMinMax(std::shared_ptr<parquet::Statistics>& stats)
      : stats_(stats)
      , index_(nullptr)
  {
  }

  // Use the page index.
  ColMinMax(std::shared_ptr<parquet::ColumnIndex>& index, std::size_t page)
      : stats_(nullptr)
      , index_(index)
      , page_index_(page)
  {
  }

  /// Any of these may be null.
  std::shared_ptr<parquet::Statistics> stats_;
  std::shared_ptr<parquet::ColumnIndex> index_;

  /// When using a column index this keeps track of which page we are
  /// currently querying.
  std::size_t page_index_ = 0;

  template <psi::DataType T> Query::Result<Query::range_t> operator()() const
  {
    using V = typename psi_to_parquet_tag<T>::scalar_type;
    using R = Query::Result<Query::range_t>;

    if (index_ != nullptr) {
      using Index = parquet::TypedColumnIndex<V>;
      std::shared_ptr<Index> index = std::static_pointer_cast<Index>(index_);
      return R(std::make_pair(index->min_values()[page_index_],
                              index->max_values()[page_index_]));
    } else if (stats_ != nullptr) {
      using Stats = parquet::TypedStatistics<V>;
      if (!stats_->HasMinMax()) return R::skip();
      std::shared_ptr<Stats> stats = std::static_pointer_cast<Stats>(stats_);
      return R(std::make_pair(stats->min(), stats->max()));
    } else {
      return R::skip();
    }
  }
};

// We don't support string searches right now.
template <>
Query::Result<Query::range_t> ColMinMax::operator()<psi::DataType::ASCII>() const
{
  return Query::Result<Query::range_t>::fail();
}

/******************************************************************************/
/**
 * Helper class for planning which row groups/pages need to be read by
 * the query executor.
 */
class Planner::Impl final {
public:
  /// Constructor.
  Impl(parquet::ParquetFileReader& reader, const Query& query)
      : metadata_(reader.metadata())
      , page_index_reader_(reader.GetPageIndexReader())
      , query_(query)
  {
  }

  /// Do the actual planning.
  const std::vector<Planner::Range>& plan();

private:
  /**
   * Create a plan for the given row group.
   */
  void plan_row_group(int32_t row_group_index);

  /**
   * Test an entire row group using column statistics.
   *
   * If the there are no statistics returns a failure.  If all the
   * rows are null returns null.  Otherwise returns the query
   * evaluation result.
   */
  Query::Result<bool>
  with_column_stats(const std::shared_ptr<parquet::RowGroupMetaData>&);

  /**
   * Test the pages inside a row group.
   */
  Query::Result<bool>
  with_page_index(const std::shared_ptr<parquet::RowGroupMetaData>&,
                  const std::shared_ptr<parquet::RowGroupPageIndexReader>&,
                  int32_t);

private:
  /**
   * Mark the given row group as needing a full scan.
   */
  void full_scan(const parquet::RowGroupMetaData&, int32_t);

  std::shared_ptr<parquet::FileMetaData> metadata_;
  std::shared_ptr<parquet::PageIndexReader> page_index_reader_;
  Query query_;
  std::vector<Planner::Range> ranges_;
};

/******************************************************************************/
Query::Result<bool> Planner::Impl::with_column_stats(
    const std::shared_ptr<parquet::RowGroupMetaData>& rg)
{
  StatsCache stats_cache;

  // Evaluation callback that tries to use column statistics.
  auto via_stats =
      [&stats_cache,
       &rg](const Query::destination_t& dest) -> Query::Result<Query::range_t> {
    auto stats = stats_cache.get(rg, dest);
    if (stats.has_value() && dest.second->data_type().has_value()) {
      return psi::dispatch(dest.second->data_type().value(),
                           ColMinMax(stats->second));
    } else {
      return Query::Result<Query::range_t>::skip();
    }
  };

  return query_.eval(via_stats);
}

/******************************************************************************/
Query::Result<bool> Planner::Impl::with_page_index(
    const std::shared_ptr<parquet::RowGroupMetaData>& rg,
    const std::shared_ptr<parquet::RowGroupPageIndexReader>& ri,
    int32_t row_group_index)
{
  IndexCache index_cache(rg, ri);

  auto via_page_index =
      [&](const Query::destination_t& dest) -> Query::Result<Query::range_t> {
    if (!dest.second->data_type().has_value())
      return Query::Result<Query::range_t>::fail();

    auto index = index_cache.get(dest);
    if (!index.has_value()) return Query::Result<Query::range_t>::skip();
    std::size_t page = index.value()->current_page;

    if (page >= index.value()->page_count) {
      // No more pages for this column.
      return Query::Result<Query::range_t>::skip();
    }

    if (index.value()->col_index->null_pages()[page]) {
      // This page only contains nulls.
      return Query::Result<Query::range_t>::skip();
    }

    index.value()->current_page += 1;
    return psi::dispatch(dest.second->data_type().value(),
                         ColMinMax(index.value()->col_index, page));
  };

  // We need to loop once for each page in the page index.  But we
  // don't know how many pages there are yet.
  Query::Result<bool> qres = query_.eval(via_page_index);
  if (qres.failed()) return qres;

  auto major_index = index_cache.max_page_count();
  if (!major_index.has_value() || major_index.value()->page_count < 1) {
    return Query::Result<bool>::fail();
  }

  // Track the query result for each page.  Start with all pages
  // marked as needed (true) so any errors or skips result in that
  // page being included.
  std::size_t page_count = major_index.value()->page_count;
  std::vector<bool> page_results(page_count, true);
  page_results[0] = !qres.is(false);

  for (std::size_t page = 1; !qres.failed() && page < page_count; ++page) {
    qres = query_.eval(via_page_index);
    page_results[page] = !qres.is(false);
  }

  // Record the page results.
  index_cache.generate_ranges(page_results, ranges_, row_group_index,
                              major_index.value());
  return qres.to(true);
}

/******************************************************************************/
void Planner::Impl::plan_row_group(int32_t row_group_index)
{
  std::shared_ptr<parquet::RowGroupPageIndexReader> row_index_reader = nullptr;

  if (page_index_reader_ != nullptr) {
    row_index_reader = page_index_reader_->RowGroup(row_group_index);
  }

  std::shared_ptr<parquet::RowGroupMetaData> rg(
      metadata_->RowGroup(row_group_index));

  // The column statistics, if present, can tell us if we are able to
  // skip an entire row group.
  auto stats_result = with_column_stats(rg);

  if (page_index_reader_ == nullptr || row_index_reader == nullptr) {
    // Record this row group if the statistic planner selected it.  If
    // the plan failed we fall back to a full row group scan and
    // record it as well.
    if (!stats_result.is(false)) {
      full_scan(*rg, row_group_index);
    }
  } else {
    // If the query wasn't interested in the row group using column
    // statistics we don't need to look at the pages.
    if (stats_result.has_value() && !stats_result.value()) {
      return;
    }

    std::size_t before_count = ranges_.size();
    auto page_res = with_page_index(rg, row_index_reader, row_group_index);

    // The page index code should have already created the necessary
    // range record even if it failed.  This is a "just in case"
    // check.
    if (!page_res.has_value() && ranges_.size() == before_count) {
      full_scan(*rg, row_group_index);
    }
  }
}

/******************************************************************************/
void Planner::Impl::full_scan(const parquet::RowGroupMetaData& rg,
                              int32_t row_group_index)
{
  // FIXME: we should emit some sort of warning.
  std::println(stderr, "no page index and no stats for rg {}, full scan needed",
               row_group_index);
  ranges_.push_back({row_group_index, 0, rg.num_rows()});
}

/******************************************************************************/
const std::vector<Planner::Range>& Planner::Impl::plan()
{
  for (int32_t i : std::views::iota(0, metadata_->num_row_groups())) {
    plan_row_group(i);
  }

  return ranges_;
}

/******************************************************************************/
Planner::Planner(parquet::arrow::FileReader& reader, const Query& query)
    : impl_(std::make_unique<Impl>(*reader.parquet_reader(), query))
{
}

/******************************************************************************/
Planner::~Planner() = default;

/******************************************************************************/
const std::vector<Planner::Range>& Planner::plan() const { return impl_->plan(); }

} // namespace MzPeak::Util
