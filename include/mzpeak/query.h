/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <functional>
#include <parquet/metadata.h>
#include <parquet/statistics.h>

#include "mzpeak/schema/psi/data_type.h"
#include "mzpeak/util/parquet.h"
#include "mzpeak/util/parquet_types.h"

namespace MzPeak {

/**
 * Helper class for finding the row groups in a Parquet file that
 * contain interesting column values.
 */
class Query final {
public:
  /// Constructor.
  Query(Util::Parquet::file_metadata_t);

  /// Destructor.
  ~Query() = default;

  /**
   * What action to take after reviewing the current row group.
   */
  enum class Action {
    /// The current row group doesn't have the value we are looking
    /// for.  Skip this row group and move on to the next.
    Skip,

    /// This row group doesn't match and we don't want to keep
    /// looking.
    Stop,

    /// This row group matches and we want to explore more row groups
    /// so keep going.
    Match,

    /// This row group matches and we want to stop looking.
    MatchStop,
  };

  using Result = std::pair<Action, std::size_t>;

  /**
   * The locations matched by the query.
   */
  struct Location {
    std::vector<std::size_t> row_group_indices;
  };

  /**
   * Helper class to fetch values from the Parquet index and return
   * query results.
   */
  class Cursor {
  public:
    /// Min and Max values from the Parquet index.
    template <typename T> using span_t = std::optional<std::pair<T, T>>;

    /// Destructor.
    ~Cursor() = default;

    /// Get the min/max values for the given column.
    template <Schema::PSI::DataType D>
    span_t<typename Schema::PSI::data_type_traits<D>::value_type>
    column_span(const std::string& path);

    /**
     * Return this value to skip the current row group.
     *
     * Use the `n` parameter to skip more than one row group.
     */
    Result skip(std::size_t n = 1) const {
      return std::make_pair<>(Action::Skip, n);
    };

    /**
     * Return this value to stop the query.
     */
    Result stop() const { return std::make_pair<>(Action::Stop, 1); };

    /**
     * Return this value to indicate the current row group matches the
     * query.  Set the `stop` parameter to `false` if query should
     * continue looking for more matching row groups.
     */
    Result match(bool stop = true) const {
      if (stop) {
        return std::make_pair<>(Action::MatchStop, 1);
      } else {
        return std::make_pair<>(Action::Match, 1);
      }
    };

  private:
    friend class Query;

    Cursor(Util::Parquet::file_metadata_t);
    bool valid() const;
    void next(std::size_t);
    bool load(const std::string&);

    Util::Parquet::file_metadata_t file_metadata_;
    std::size_t num_row_groups_;

    std::size_t row_group_idx_ = 0;
    std::unique_ptr<parquet::RowGroupMetaData> row_group_meta_ = nullptr;

    std::size_t col_chunk_idx_ = 0;
    std::unique_ptr<parquet::ColumnChunkMetaData> col_chunk_ = nullptr;
    std::shared_ptr<parquet::Statistics> col_stats_ = nullptr;
  };

  /**
   * Find matching row groups by providing a query function.
   */
  const Location& find(std::function<Result(Cursor&)>);

private:
  Util::Parquet::file_metadata_t file_metadata_;
  Location location_;
};

/******************************************************************************/
template <Schema::PSI::DataType D>
Query::Cursor::span_t<typename Schema::PSI::data_type_traits<D>::value_type>
Query::Cursor::column_span(const std::string& path) {
  if (!load(path)) return {};
  auto tptr(Util::parquet_statistics_cast<D>(*col_chunk_, *col_stats_));
  return std::make_pair<>(tptr->min(), tptr->max());
}

} // namespace MzPeak
