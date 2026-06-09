/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <arrow/buffer.h>
#include <arrow/io/api.h>
#include <arrow/io/buffered.h>
#include <memory>
#include <parquet/properties.h>

#include "mzpeak/exception.h"
#include "mzpeak/util/arrow.h"

namespace MzPeak::Util {

/******************************************************************************/
/**
 * Arrow file access.
 */
class ArrowFile_ final : public arrow::io::RandomAccessFile {
public:
  /// Constructor.
  ArrowFile_(std::shared_ptr<File> file)
      : file_(std::move(file))
  {
    arrow::Result<std::unique_ptr<arrow::ResizableBuffer>> res(
        arrow::AllocateResizableBuffer(parquet::kDefaultFooterReadSize));

    if (res.ok()) {
      buffer_ = std::move(res.ValueOrDie());
    } else {
      throw ParquetError(res.status().ToString());
    }
  }

  /// Destructor.
  ~ArrowFile_() = default;

  /// Return the total file size in bytes.
  arrow::Result<int64_t> GetSize() { return file_->size(); }

  /// Seek in file/stream.
  arrow::Status Seek(int64_t position)
  {
    if (!file_->seek(position)) {
      std::string msg("unable to seek");
      return arrow::Status(arrow::StatusCode::IOError, msg);
    }

    return arrow::Status::OK();
  }

  /// Report the current position.
  arrow::Result<int64_t> Tell() const
  {
    std::optional<std::size_t> n = file_->tell();

    if (n.has_value()) {
      return arrow::Result<int64_t>(n.value());
    } else {
      // Arrow error result:
      return arrow::Result<int64_t>();
    }
  }

  /// Read data from current file position.
  arrow::Result<int64_t> Read(int64_t nbytes, void* out)
  {
    std::optional<std::size_t> n = file_->read(static_cast<uint8_t*>(out), nbytes);

    if (n.has_value()) {
      return arrow::Result<int64_t>(n.value());
    } else {
      return arrow::Result<int64_t>();
    }
  }

  /// Read into a buffer.
  arrow::Result<std::shared_ptr<arrow::Buffer>> Read(int64_t nbytes)
  {
    using arrow_buffer_t = std::shared_ptr<arrow::Buffer>;

    if (nbytes > buffer_->capacity()) {
      arrow::Status status = buffer_->Resize(nbytes, true);
      if (!status.ok()) throw ParquetError(status.ToString());
      buffer_->ZeroPadding();
    }

    std::optional<std::size_t> n = file_->read(buffer_->mutable_data(), nbytes);

    if (n.has_value()) {
      auto slice = arrow::SliceBuffer(buffer_, 0, *n);
      return arrow::Result<arrow_buffer_t>(slice);
    } else {
      return arrow::Result<arrow_buffer_t>();
    }
  }

  /// Close the file/stream.
  arrow::Status Close()
  {
    file_->close();
    return buffer_->Resize(0, true);
  }

  /// Return `true` if the file/stream is closed.
  bool closed() const { return !file_->is_open(); }

private:
  std::shared_ptr<File> file_;
  std::shared_ptr<arrow::ResizableBuffer> buffer_;
};

/******************************************************************************/
struct Arrow::Impl {
  Impl(std::unique_ptr<File> file)
      : file_(std::move(file))
      , reader_(std::make_shared<ArrowFile_>(file_))
  {
  }

  std::shared_ptr<File> file_;
  std::shared_ptr<ArrowFile_> reader_;
};

/******************************************************************************/
Arrow::Arrow(std::unique_ptr<File> file)
    : impl_(std::make_unique<Impl>(std::move(file)))
{
}

/******************************************************************************/
Arrow::~Arrow() = default;

/******************************************************************************/
std::shared_ptr<Arrow::random_access_t> Arrow::reader() const
{
  return impl_->reader_;
}

} // namespace MzPeak::Util
