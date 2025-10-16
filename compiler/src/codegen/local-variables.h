//
// Created by lunbun on 6/12/2022.
//

#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "context/context.h"
#include "types/type.h"

namespace magnetic::codegen {

class TypedLocal {
 public:
  TypedLocal(llvm::Value *ptr, Type type) : ptr_(ptr), type_(type) {}

  [[nodiscard]] Value EmitLoad(Context *ctx, llvm::IRBuilder<> &builder, const std::string &name) const;
  void EmitStore(llvm::IRBuilder<> &builder, Value value) const;

 private:
  llvm::Value *ptr_;
  Type type_;
};

struct VirtualLocal {
  std::map<Type, TypedLocal> typed;
};

class LocalVariables {
 public:
  LocalVariables(Context *ctx, llvm::BasicBlock *alloca_block);

  TypedLocal &Get(int32_t index, Type kind);
  TypedLocal &GetInt(int32_t index);
  TypedLocal &GetLong(int32_t index);
  TypedLocal &GetFloat(int32_t index);
  TypedLocal &GetDouble(int32_t index);
  TypedLocal &GetObject(int32_t index);

  [[nodiscard]] llvm::BasicBlock *alloca_block() const { return this->alloca_block_; }

 private:
  Context *ctx_;
  llvm::BasicBlock *alloca_block_;
  llvm::IRBuilder<> builder_;
  std::map<int32_t, VirtualLocal> locals_;
};

}// namespace magnetic::codegen
