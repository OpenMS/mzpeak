/*

This file is part of the mzpeak project.  It is subject to the license
specified in the LICENSE file which can be found in the top-level
directory of this repository.

*/

#include <zip.h>

#include "mzpeak/zip.h"

namespace MzPeak {

/******************************************************************************/
/**
 * A streaming source for files in a zip archive.
 */
class ZipFile_ final : public MzPeak::File {
public:
  ZipFile_(zip_file_t* file, std::size_t size, fs::path path)
      : impl_(std::make_shared<Impl>(file, size, path)) {}

  std::string name() const { return impl_->path_; };

  std::size_t size() const { return impl_->size_; };

  std::optional<std::size_t> read(uint8_t* buf, std::size_t size) {
    if (buf == nullptr || size == 0 || !is_open()) return {};

    zip_int64_t n = zip_fread(impl_->file_, buf, size);

    if (n <= 0) {
      return {};
    } else {
      return n;
    }
  }

  std::optional<std::size_t> tell() const {
    zip_int64_t n = zip_ftell(impl_->file_);

    if (n < 0) {
      return {};
    } else {
      return n;
    }
  };

  bool seek(std::size_t pos) {
    zip_int8_t errnum = zip_fseek(impl_->file_, pos, SEEK_SET);
    return errnum == 0;
  };

  void close() { impl_->close(); }
  bool is_open() const { return impl_->file_ != nullptr; }

private:
  class Impl {
  public:
    Impl(zip_file_t* file, std::size_t size, fs::path path)
        : size_(size), file_(file), path_(path) {};
    ~Impl() { close(); }

    void close() {
      if (file_ != nullptr) {
        zip_fclose(file_);
        file_ = nullptr;
      }
    };

    std::size_t size_;
    zip_file_t* file_;
    fs::path path_;

  private:
    Impl(const Impl&) = default;
  };

  std::shared_ptr<Impl> impl_;
};

/******************************************************************************/
struct Zip::Impl {

  /**************************************************************************/
  Impl(const fs::path& path) : archive(nullptr) {
    int errnum{};
    archive = zip_open(path.c_str(), ZIP_RDONLY, &errnum);

    if (archive == nullptr) {
      error("failed to open zip archive ", errnum);
    }
  }

  /**************************************************************************/
  ~Impl() {
    if (archive != nullptr) {
      zip_close(archive);
      archive = nullptr;
    }
  }

  /**************************************************************************/
  void error(const std::string& msg, const std::optional<int>& errnum) {
    std::string m(msg);
    zip_error_t error;
    zip_error_t* error_ptr;

    if (errnum) {
      zip_error_init_with_code(&error, *errnum);
      error_ptr = &error;
    } else {
      error_ptr = zip_get_error(archive);
    }

    m += zip_error_strerror(error_ptr);
    throw(std::invalid_argument(m));
  };

  /**************************************************************************/
  void error_open(const fs::path& path, const std::optional<int>& errnum) {
    std::string msg("failed to open file in zip archive ");
    msg += path.string() + ": ";
    error(msg, errnum);
  };

  /**************************************************************************/
  zip_t* archive;
};

/******************************************************************************/
Zip::Zip(const fs::path& path) : impl_(std::make_unique<Impl>(path)) {}

/******************************************************************************/
// NOTE: This is needed due to the pimpl pattern and the `Impl` type
// not being complete in the header file :(
Zip::~Zip() = default;

/******************************************************************************/
std::vector<fs::path> Zip::list() {
  zip_int64_t num = zip_get_num_entries(impl_->archive, ZIP_FL_UNCHANGED);
  zip_stat_t stat;
  int errnum;

  std::vector<fs::path> files;
  files.reserve(num);

  for (zip_int64_t index = 0; index < num; ++index) {
    errnum = zip_stat_index(impl_->archive, index, ZIP_FL_UNCHANGED, &stat);

    if (errnum == 0 && stat.valid & ZIP_STAT_NAME) {
      files.push_back(stat.name);
    }
  }

  return files;
}

/******************************************************************************/
std::unique_ptr<MzPeak::File> Zip::read_file(const fs::path& path) {
  zip_stat_t stat;
  int errnum = zip_stat(impl_->archive, path.c_str(), ZIP_FL_UNCHANGED, &stat);

  if (errnum != 0) {
    impl_->error_open(path, errnum);
  } else if (!(stat.valid & ZIP_STAT_SIZE)) {
    impl_->error_open(path, {});
  }

  zip_file_t* file = zip_fopen(impl_->archive, path.c_str(), 0);

  if (file == nullptr) {
    impl_->error_open(path, {});
  }

  return std::make_unique<ZipFile_>(file, stat.size, path);
}

} // namespace MzPeak
