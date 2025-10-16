//
// Created by lunbun on 7/16/2022.
//

#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "type.h"

namespace magnetic {

class IArrayInfo {
 public:
  [[nodiscard]] static IArrayInfo *Get(Context *ctx, Type element);

  IArrayInfo(const IArrayInfo &) = delete;
  IArrayInfo &operator=(const IArrayInfo &) = delete;
  IArrayInfo(IArrayInfo &&) = delete;
  IArrayInfo &operator=(IArrayInfo &) = delete;
  virtual ~IArrayInfo() noexcept = default;

  [[nodiscard]] virtual Value EmitGetLength(llvm::IRBuilder<> &builder, Value array_ref) const;
  [[nodiscard]] virtual Value EmitGetElement(llvm::IRBuilder<> &builder, Value array_ref, Value index) const;

  [[nodiscard]] virtual Type element_type() const = 0;

 protected:
  IArrayInfo() = default;
};

}// namespace magnetic
