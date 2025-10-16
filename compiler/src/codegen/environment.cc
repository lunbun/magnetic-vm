//
// Created by lunbun on 6/10/2022.
//

#include "environment.h"

#include <cjbp/cjbp.h>
#include <llvm/IR/IRBuilder.h>

#include "class/class.h"
#include "compilation-unit/compilation-unit.h"

namespace magnetic::codegen {

void Stack::Push(Value value) { this->container_.push_back(value); }
Value Stack::Pop() {
  Value top = this->container_.back();
  this->container_.pop_back();
  return top;
}
const Value &Stack::Top() const { return this->container_.back(); }

Environment::Environment(ClassInfo *owner, MethodDeclaration *method, cjbp::Method *bytecode, llvm::Function *function,
                         llvm::Module *module)
    : module_(module), method_(method), stack_(), iterator_(nullptr), locals_(nullptr), cfg_(nullptr),
      builder_(nullptr) {
  this->class_ = owner;
  this->function_ = function;
  this->builder_ = std::make_unique<llvm::IRBuilder<>>(*this->ctx()->llvm_ctx());
  this->iterator_ = std::make_unique<cjbp::CodeIterator>(*bytecode->code_attribute());
  this->cfg_ = std::make_unique<ControlFlowGraph>(this->iterator());
  llvm::BasicBlock *alloca_block = llvm::BasicBlock::Create(*this->ctx()->llvm_ctx(), "alloca", this->function_);
  this->locals_ = std::make_unique<LocalVariables>(this->ctx(), alloca_block);
}
Context *Environment::ctx() const { return this->class_->ctx(); }

}// namespace magnetic::codegen
