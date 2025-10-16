//
// Created by lunbun on 6/10/2022.
//

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>

#include <cjbp/cjbp.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "types/type.h"

namespace magnetic {

class Context;
class ClassInfo;
class CompilationUnit;
class StructElementLayoutSpecifier;

class FieldDeclaration {
 public:
  static std::unique_ptr<FieldDeclaration> Create(Context *ctx, bool is_static, std::string_view class_name,
                                                  std::string_view name, const std::string &descriptor);

  FieldDeclaration(const FieldDeclaration &) = delete;
  FieldDeclaration &operator=(const FieldDeclaration &) = delete;
  FieldDeclaration(FieldDeclaration &&) = delete;
  FieldDeclaration &operator=(FieldDeclaration &&) = delete;
  virtual ~FieldDeclaration() noexcept = default;

  virtual void EmitDefinition(llvm::Module *module) = 0;

  virtual Value EmitLoad(llvm::IRBuilder<> &builder, std::optional<Value> object_ref, const std::string &name) = 0;
  virtual void EmitStore(llvm::IRBuilder<> &builder, std::optional<Value> object_ref, Value value) = 0;

  [[nodiscard]] virtual StructElementLayoutSpecifier *element_layout() = 0;

  [[nodiscard]] Context *ctx() const { return this->ctx_; }
  [[nodiscard]] Type descriptor() const { return this->descriptor_; }

 protected:
  FieldDeclaration(Context *ctx, const std::string &descriptor);

 private:
  Context *ctx_;
  Type descriptor_;
};

}// namespace magnetic
