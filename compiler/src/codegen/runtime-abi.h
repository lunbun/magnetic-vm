//
// Created by lunbun on 7/12/2022.
//

#pragma once

#include <memory>
#include <string>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "types/type.h"

namespace magnetic {

class Context;

class RuntimeABI {
 public:
  static std::unique_ptr<RuntimeABI> CreateDefaultABI();

  RuntimeABI(const RuntimeABI &) = delete;
  RuntimeABI &operator=(const RuntimeABI &) = delete;
  RuntimeABI(RuntimeABI &&) = delete;
  RuntimeABI &operator=(RuntimeABI &&) = delete;
  virtual ~RuntimeABI() noexcept = default;

  void set_ctx(Context *ctx) { this->ctx_ = ctx; }

  /**
   * Emits IR to load a string constant.
   * @return the LLVM value with a pointer to a java.lang.String object
   */
  virtual Value GetStringConstant(llvm::IRBuilder<> &builder, std::string_view value) = 0;

 protected:
  RuntimeABI();

  [[nodiscard]] Context *ctx() const { return this->ctx_; }

 private:
  Context *ctx_;
};

}// namespace magnetic
