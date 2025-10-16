//
// Created by lunbun on 6/9/2022.
//

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <cjbp/cjbp.h>

namespace magnetic {

class ClassPath {
 public:
  static std::unique_ptr<ClassPath> CreateCompositeClassPath(std::vector<std::unique_ptr<ClassPath>> paths);
  static std::unique_ptr<ClassPath> CreateDirectoryClassPath(const std::string &path);
  static std::unique_ptr<ClassPath> CreateJarClassPath(const std::string &path);

  virtual ~ClassPath() noexcept = default;

  /**
   * @return nullptr if the class isn't in this class path.
   */
  virtual std::unique_ptr<cjbp::DataInputStream> Find(const std::string &name) = 0;
};

}// namespace magnetic
