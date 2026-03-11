/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <memory>
#include <optional>

#include "mzpeak/buffer.h"

namespace MzPeak::File {

/******************************************************************************/
/**
 * An interface for files that can be read from.
 */
class Readable {
public:
  using buffer_t = std::shared_ptr<Buffer::Base>;

  /// Destructor.
  virtual ~Readable() {};

  /**
   * Return the size of the file, in bytes.
   */
  virtual std::size_t size() const = 0;

  /**
   * Return the position of the read pointer.
   */
  virtual std::optional<std::size_t> tell() const = 0;

  /**
   * Read from the file.
   *
   * If successful, return the number of bytes actually read.
   */
  virtual std::optional<std::size_t> read(uint8_t* buffer, std::size_t size) = 0;

  /**
   * Read from the file.
   *
   * If successful, return the number of bytes actually read.
   *
   * The default implementation uses a simple memory buffer.
   */
  virtual std::optional<buffer_t> read(std::size_t size);

  /**
   * Move the read pointer to the given file position.
   *
   * Returns `true` if the seek was successful.
   */
  virtual bool seek(std::size_t pos) = 0;

  /**
   * Close the file.
   */
  virtual void close() = 0;

  /**
   * Returns `true` if the file is open.
   */
  virtual bool is_open() const = 0;

protected:
  // Prevent construction.
  Readable() = default;

private:
  // Prevent copying.
  Readable(const Readable&) = default;
};

/******************************************************************************/
/**
 * Turn a `File::Readable` object into an `std::istream`.
 */
std::unique_ptr<std::istream> to_istream(std::unique_ptr<Readable>);

} // namespace MzPeak::File
