//
// Created by lunbun on 6/9/2022.
//

#include "path.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>

#include <zip/zip.h>

namespace magnetic {

namespace {
class CompositeClassPath : public ClassPath {
 public:
  explicit CompositeClassPath(std::vector<std::unique_ptr<ClassPath>> paths) : paths_(std::move(paths)) {}
  ~CompositeClassPath() noexcept override = default;

  /**
   * @return nullptr if the class isn't in this class path.
   */
  std::unique_ptr<cjbp::DataInputStream> Find(const std::string &name) override {
    for (const auto &path : this->paths_) {
      auto stream = path->Find(name);
      if (stream != nullptr) return stream;
    }
    return nullptr;
  }

 private:
  std::vector<std::unique_ptr<ClassPath>> paths_;
};

class DirectoryClassPath : public ClassPath {
 public:
  explicit DirectoryClassPath(const std::string &path) : path_(path) {}
  ~DirectoryClassPath() noexcept override = default;

  /**
   * @return nullptr if the class isn't in this class path.
   */
  std::unique_ptr<cjbp::DataInputStream> Find(const std::string &name) override {
    std::string rel_path = name;
    std::replace(rel_path.begin(), rel_path.end(), '.', '/');
    rel_path.append(".class");
    std::filesystem::path full_path = (this->path_ / rel_path);

    auto ifs = std::make_unique<std::ifstream>(full_path, std::ios::in | std::ios::binary);
    if (ifs->fail()) return nullptr;
    return std::make_unique<cjbp::FileInputStream>(std::move(ifs));
  }

 private:
  std::filesystem::path path_;
};

class JarClassPath : public ClassPath {
 public:
  explicit JarClassPath(const std::string &path) { this->zip_ = zip_open(path.c_str(), 0, 'r'); }
  ~JarClassPath() noexcept override { zip_close(this->zip_); }

  /**
   * @return nullptr if the class isn't in this class path.
   */
  std::unique_ptr<cjbp::DataInputStream> Find(const std::string &name) override {
    std::string path = name;
    std::replace(path.begin(), path.end(), '.', '/');
    path.append(".class");

    std::unique_ptr<cjbp::DataInputStream> stream;
    zip_entry_open(this->zip_, path.c_str());

    size_t bufsize = zip_entry_size(this->zip_);
    std::vector<uint8_t> buf(bufsize);
    ssize_t result = zip_entry_noallocread(this->zip_, buf.data(), bufsize);
    if (result >= 0) {
      assert(result == bufsize);
      stream = std::make_unique<cjbp::ByteInputStream>(std::move(buf));
    }
    zip_entry_close(this->zip_);
    return stream;
  }

 private:
  zip_t *zip_;
};
}// namespace

std::unique_ptr<ClassPath> ClassPath::CreateCompositeClassPath(std::vector<std::unique_ptr<ClassPath>> paths) {
  return std::make_unique<CompositeClassPath>(std::move(paths));
}
std::unique_ptr<ClassPath> ClassPath::CreateDirectoryClassPath(const std::string &path) {
  return std::make_unique<DirectoryClassPath>(path);
}
std::unique_ptr<ClassPath> ClassPath::CreateJarClassPath(const std::string &path) {
  return std::make_unique<JarClassPath>(path);
}

}// namespace magnetic
