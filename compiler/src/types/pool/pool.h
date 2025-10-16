//
// Created by lunbun on 6/9/2022.
//

#pragma once

#include <memory>
#include <unordered_map>

#include "path.h"

namespace magnetic {

class Context;
class ClassInfo;

class ClassPool {
 public:
  explicit ClassPool(std::unique_ptr<ClassPath> path) : ctx_(nullptr), path_(std::move(path)), classes_() {}
  ~ClassPool() noexcept;

  /**
   * @return nullptr if the class doesn't exist
   */
  ClassInfo *Get(const std::string &class_name);

  Context *ctx() const { return this->ctx_; }
  void set_ctx(Context *ctx) { this->ctx_ = ctx; }

 private:
  Context *ctx_;
  std::unique_ptr<ClassPath> path_;
  std::unordered_map<std::string, std::unique_ptr<ClassInfo>> classes_;
};

}// namespace magnetic
