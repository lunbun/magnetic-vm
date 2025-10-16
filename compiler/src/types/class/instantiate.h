//
// Created by lunbun on 7/7/2022.
//

#pragma once

#include <map>
#include <memory>
#include <string>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>

#include "types/type.h"

namespace magnetic {

class Context;
class ClassInfo;

class ClassInstantiator {
 public:
  static std::unique_ptr<ClassInstantiator> Create(Context *ctx, std::string class_name);

  ClassInstantiator() = delete;
  ClassInstantiator(const ClassInstantiator &) = delete;
  ClassInstantiator &operator=(const ClassInstantiator &) = delete;
  ClassInstantiator(Context *ctx, std::string class_name);

  void EmitDefinition(llvm::Module *module);
  Value EmitInstantiation(llvm::IRBuilder<> &builder, const std::string &name);

  void set_owner(ClassInfo *owner) { this->owner_ = owner; }

 private:
  Context *ctx_;
  std::string class_name_;
  std::string mangled_name_;
  ClassInfo *owner_;// Can be nullptr.

  std::map<llvm::Module *, llvm::Function *> instantiators_;

  llvm::Function *GetInstantiatorInModule(llvm::Module *module);
};

}// namespace magnetic
