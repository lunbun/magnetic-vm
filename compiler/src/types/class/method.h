//
// Created by lunbun on 6/10/2022.
//

#pragma once

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <cjbp/cjbp.h>
#include <llvm/IR/Function.h>

#include "descriptor.h"
#include "types/type.h"

namespace magnetic {

class Context;
class ClassInfo;
class CompilationUnit;

class MethodDeclaration {
 public:
  static std::unique_ptr<MethodDeclaration> Create(Context *ctx, bool is_static, std::string class_name,
                                                   std::string name, std::string descriptor);

  MethodDeclaration() = delete;
  MethodDeclaration(const MethodDeclaration &) = delete;
  MethodDeclaration &operator=(const MethodDeclaration &) = delete;
  MethodDeclaration(Context *ctx, bool is_static, std::string class_name, std::string name, std::string descriptor);

  void EmitDefinition(llvm::Module *module);

  [[nodiscard]] Value EmitCall(llvm::IRBuilder<> &builder, std::optional<Value> object_ref,
                               const std::vector<Value> &params, const std::string &name);
  [[nodiscard]] Value EmitVirtualCall(llvm::IRBuilder<> &builder, Value object_ref, const std::vector<Value> &params,
                                      const std::string &name);

  [[nodiscard]] bool is_static() const;
  [[nodiscard]] bool CanBeOverridden() const;
  [[nodiscard]] bool IsVirtual() const;

  [[nodiscard]] llvm::Function *GetFunctionInModule(llvm::Module *module);

  [[nodiscard]] MethodDescriptor *descriptor() const { return this->descriptor_.get(); }
  [[nodiscard]] const std::string &class_name() const { return this->class_name_; }
  [[nodiscard]] const std::string &name() const { return this->name_; }
  [[nodiscard]] const std::string &raw_descriptor() const { return this->raw_descriptor_; }
  void set_owner(ClassInfo *owner) { this->owner_ = owner; }
  void set_bytecode(cjbp::Method *bytecode) { this->bytecode_ = bytecode; }

 private:
  Context *ctx_;
  std::string mangled_name_, virtual_mangled_name_;
  std::string class_name_, name_, raw_descriptor_;
  std::unique_ptr<MethodDescriptor> descriptor_;
  llvm::FunctionType *function_type_;
  ClassInfo *owner_;      // Can be nullptr.
  cjbp::Method *bytecode_;// Can be nullptr.

  std::map<llvm::Module *, llvm::Function *> functions_;
  std::map<llvm::Module *, llvm::Function *> virtual_dispatches_;

  [[nodiscard]] bool is_final() const;
  [[nodiscard]] bool is_constructor() const;

  void EmitVirtualDispatchThunkDefinition(llvm::Module *module);

  llvm::Function *CreateFunctionInModule(llvm::Module *module, const std::string &mangled_name) const;
  [[nodiscard]] llvm::Function *GetVirtualDispatchInModule(llvm::Module *module);
};

}// namespace magnetic
