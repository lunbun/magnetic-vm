//
// Created by lunbun on 6/10/2022.
//

#pragma once

#include <memory>
#include <vector>

#include <llvm/IR/Value.h>
#include <llvm/IR/IRBuilder.h>
#include <cjbp/cjbp.h>

#include "cfg/control-flow-graph.h"
#include "class/method.h"
#include "context/context.h"
#include "local-variables.h"
#include "types/type.h"

namespace magnetic::codegen {

class Stack {
 public:
  void Push(Value value);
  Value Pop();
  const Value &Top() const;

 private:
  std::vector<Value> container_;
};

class Environment {
 public:
  Environment(ClassInfo *owner, MethodDeclaration *method, cjbp::Method *bytecode, llvm::Function *function, llvm::Module *module);

  [[nodiscard]] Context *ctx() const;
  [[nodiscard]] ClassInfo *clazz() const { return this->class_; }
  [[nodiscard]] llvm::Module *module() const { return this->module_; }
  [[nodiscard]] MethodDeclaration *method() const { return this->method_; }
  [[nodiscard]] llvm::Function *function() const { return this->function_; }
  [[nodiscard]] Stack &stack() { return this->stack_; }
  [[nodiscard]] cjbp::CodeIterator &iterator() const { return *this->iterator_; }
  [[nodiscard]] LocalVariables &locals() const { return *this->locals_; }
  [[nodiscard]] ControlFlowGraph &cfg() const { return *this->cfg_; }
  [[nodiscard]] llvm::IRBuilder<> &builder() const { return *this->builder_; }

 private:
  ClassInfo *class_;
  llvm::Module *module_;
  MethodDeclaration *method_;
  llvm::Function *function_;
  Stack stack_;
  std::unique_ptr<cjbp::CodeIterator> iterator_;
  std::unique_ptr<LocalVariables> locals_;
  std::unique_ptr<ControlFlowGraph> cfg_;
  std::unique_ptr<llvm::IRBuilder<>> builder_;
};

}
