/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#pragma once

#include <cstdint>

namespace MzPeak::Buffer {

/******************************************************************************/
/**
 * An interface for buffers that can be used to move bytes around
 * without copying.
 */
class Base {
public:
  /// Destructor.
  virtual ~Base() {};

  /**
   * Return the total number of bytes that the buffer can store.
   */
  virtual std::size_t capacity() const = 0;

  /**
   * Return the buffer's size in bytes.  This is the amount of
   * readable data occupying in the buffer.
   */
  virtual std::size_t size() const = 0;

  /**
   * Update the size of the buffer.  The parameter `set` represents
   * the number of bytes in the buffer that can be read from.
   */
  virtual void size(std::size_t set) = 0;

  /**
   * Return a pointer the the buffer's memory.
   */
  virtual uint8_t* data() = 0;

protected:
  /// Constructor.
  Base() = default;
};

/******************************************************************************/
/**
 * A simple buffer using an array.
 */
template <std::size_t N = 64 * 1024> class Basic final : public Base {
public:
  Basic() {};
  std::size_t capacity() const { return N; };
  std::size_t size() const { return size_; };
  void size(std::size_t set) { size_ = set; };
  uint8_t* data() { return buffer_; };

private:
  uint8_t buffer_[N];
  std::size_t size_ = 0;
};

} // namespace MzPeak::Buffer
