//
// Created by lunbun on 6/9/2022.
//

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <cjbp/cjbp.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include "layout.h"
#include "vtable.h"

namespace magnetic {

class CompilationUnit;
class FieldDeclaration;
class MethodDeclaration;

class ClassInfo {
 public:
  ClassInfo(Context *ctx, std::unique_ptr<cjbp::Class> bytecode, std::shared_ptr<CompilationUnit> compilation_unit);
  ClassInfo(const ClassInfo &) = delete;
  ClassInfo &operator=(const ClassInfo &) = delete;
  ClassInfo(ClassInfo &&) = default;
  ClassInfo &operator=(ClassInfo &&) = default;
  ~ClassInfo() noexcept;

  void EmitDefinition();

  [[nodiscard]] bool IsSubClassOf(const ClassInfo *other) const;

  [[nodiscard]] Value EmitUncheckedClassCastTo(llvm::IRBuilder<> &builder, const ClassInfo *dest,
                                               llvm::Value *ptr) const;

  [[nodiscard]] Context *ctx() const { return this->ctx_; }
  [[nodiscard]] const std::string &name() const;
  [[nodiscard]] cjbp::Class *bytecode() const { return this->bytecode_.get(); }
  [[nodiscard]] CompilationUnit *compilation_unit() const { return this->compilation_unit_.get(); }
  [[nodiscard]] llvm::StructType *struct_type() const { return this->struct_type_; }
  [[nodiscard]] ClassInfo *super_class() const { return this->super_class_; }
  [[nodiscard]] const VTable &vtable() const;
  [[nodiscard]] bool is_final() const;

 private:
  Context *ctx_;
  std::unique_ptr<cjbp::Class> bytecode_;
  std::shared_ptr<CompilationUnit> compilation_unit_;
  llvm::StructType *struct_type_;

  ClassInfo *super_class_;// Can be nullptr.
  std::optional<VTable> vtable_;
  std::optional<StructElementLayoutSpecifier> super_class_layout_;

  [[nodiscard]] std::optional<ssize_t> GetCastOffset(const ClassInfo *dest) const;
};

}// namespace magnetic
