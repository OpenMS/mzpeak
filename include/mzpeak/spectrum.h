/*

This file is part of the mzpeak.h project.  It is subject to the
license specified in the LICENSE file which can be found in the
top-level directory of this repository.

*/

#pragma once

#include <memory>

#include "mzpeak/data/encoding.h"
#include "mzpeak/schema/psi/data_type.h"

namespace MzPeak {

// Forward declaration.
class Spectra;

/**
 * Access to a single spectrum in an MzPeak file.
 */
class Spectrum final {
public:
  /// Ensure types stay in sync.
  using mz_type =
      Schema::PSI::data_type_traits<Schema::PSI::DataType::Float64>::value_type;

  /// Ensure types stay in sync.
  using intensity_type =
      Schema::PSI::data_type_traits<Schema::PSI::DataType::Int32>::value_type;

  /// Destructor.
  ~Spectrum() = default;

  /**
   * Mass-to-charge values.
   */
  const std::vector<mz_type>& mz() const;

  /**
   * Intensity values.
   */
  const std::vector<intensity_type>& intensity() const;

  // FIXME: level?

  /**
   * Raw access to the remaining values stored in the MzPeak file for
   * this spectrum.
   *
   * These values needed to be coded using the Encoding class.  The
   * array index will also be needed.
   *
   * NOTE: Mass-to-charge and intensity values have been removed from
   * the returned object.  Use the corresponding methods in this
   * class to fetch those values instead.
   */
  const Data::array_map_type& raw_encoded_arrays() const;

  /**
   * The array index for this spectrum.
   */
  const Schema::ArrayIndex& array_index() const;

protected:
  friend class Spectra;

  /// Internal constructor.
  Spectrum(const Schema::ArrayIndex&, std::unique_ptr<Data::array_map_type>);

private:
  Schema::ArrayIndex array_index_;
  std::shared_ptr<Data::array_map_type> map_;
  std::vector<mz_type> mz_;
  std::vector<intensity_type> intensity_;
};

} // namespace MzPeak
