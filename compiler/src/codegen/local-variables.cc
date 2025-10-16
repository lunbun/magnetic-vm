//
// Created by lunbun on 6/12/2022.
//

#include "local-variables.h"

#include <cassert>
#include <string>

#include <fmt/core.h>

namespace magnetic::codegen {

Value TypedLocal::EmitLoad(Context *ctx, llvm::IRBuilder<> &builder, const std::string &name) const {
  llvm::Value *value = builder.CreateLoad(this->type_.llvm_type(ctx), this->ptr_, name);
  return {value, this->type_};
}
void TypedLocal::EmitStore(llvm::IRBuilder<> &builder, Value value) const {
  assert(value.type == this->type_);
  builder.CreateStore(value.value, this->ptr_);
}

LocalVariables::LocalVariables(Context *ctx, llvm::BasicBlock *alloca_block)
    : ctx_(ctx), alloca_block_(alloca_block), builder_(*ctx->llvm_ctx()), locals_() {}

TypedLocal &LocalVariables::Get(int32_t index, Type type) {
  if (this->locals_.find(index) == this->locals_.end()) { this->locals_.emplace(index, VirtualLocal()); }
  VirtualLocal &virtual_local = this->locals_.at(index);

  if (virtual_local.typed.find(type) == virtual_local.typed.end()) {
    this->builder_.SetInsertPoint(this->alloca_block(), this->alloca_block()->begin());
    std::string local_name = (type.name() + std::to_string(index));
    llvm::Value *ptr = this->builder_.CreateAlloca(type.llvm_type(this->ctx_), nullptr, local_name);
    virtual_local.typed.emplace(type, TypedLocal(ptr, type));
  }
  return virtual_local.typed.at(type);
}
TypedLocal &LocalVariables::GetInt(int32_t index) { return this->Get(index, Type::kInt); }
TypedLocal &LocalVariables::GetLong(int32_t index) { return this->Get(index, Type::kLong); }
TypedLocal &LocalVariables::GetFloat(int32_t index) { return this->Get(index, Type::kFloat); }
TypedLocal &LocalVariables::GetDouble(int32_t index) { return this->Get(index, Type::kDouble); }
TypedLocal &LocalVariables::GetObject(int32_t index) { return this->Get(index, Type::kObject); }

}// namespace magnetic::codegen
