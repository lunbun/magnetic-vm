//
// Created by lunbun on 6/10/2022.
//

#include "method.h"

#include <string>

#include <cjbp/cjbp.h>
#include <llvm/IR/Function.h>

#include "class/descriptor.h"
#include "codegen/codegen-method.h"
#include "compilation-unit/compilation-unit.h"
#include "context/context.h"
#include "types/mangle.h"

namespace magnetic {

std::unique_ptr<MethodDeclaration> MethodDeclaration::Create(Context *ctx, bool is_static, std::string class_name,
                                                             std::string name, std::string descriptor) {
  return std::make_unique<MethodDeclaration>(ctx, is_static, std::move(class_name), std::move(name),
                                             std::move(descriptor));
}
MethodDeclaration::MethodDeclaration(Context *ctx, bool is_static, std::string class_name, std::string name,
                                     std::string descriptor)
    : ctx_(ctx), functions_(), class_name_(std::move(class_name)), name_(std::move(name)),
      raw_descriptor_(std::move(descriptor)), owner_(nullptr), bytecode_(nullptr) {
  this->mangled_name_ = ctx->name_mangler()->MangleMethodName(this->class_name_, this->name_, this->raw_descriptor_);
  this->virtual_mangled_name_ =
      ctx->name_mangler()->MangleVirtualDispatchName(this->class_name_, this->name_, this->raw_descriptor_);
  this->descriptor_ = ParseMethodDescriptor(ctx, is_static, this->raw_descriptor_, false);
  this->function_type_ = this->descriptor_->CreateFunctionType(ctx);
}
bool MethodDeclaration::is_static() const { return this->descriptor_->is_static(); }
bool MethodDeclaration::is_final() const {
  assert(this->bytecode_ != nullptr);
  return (this->bytecode_->access_flags() & cjbp::AccessFlags::kFinal);
}
bool MethodDeclaration::is_constructor() const { return (this->name_ == "<init>") && (this->raw_descriptor_ == "()V"); }
bool MethodDeclaration::CanBeOverridden() const {
  if (this->is_static()) return false;
  if (this->is_constructor()) return false;

  // Assume it can be overridden if there's not enough information.
  if (this->bytecode_ == nullptr) return true;
  if (this->owner_ == nullptr) return true;

  if (this->is_final()) return false;
  if (this->owner_->is_final()) return false;
  return true;
}
bool MethodDeclaration::IsVirtual() const {
  if (this->is_static()) return false;
  if (this->is_constructor()) return false;

  // Assume it is virtual if there's not enough information.
  if (this->owner_ == nullptr) return true;

  // Method doesn't have to be virtual if it can't be overridden, and it doesn't exist in the super class (or if it has
  // no super class).
  if (!this->CanBeOverridden()) {
    if (this->owner_->super_class() == nullptr) return false;
    if (!this->owner_->super_class()->vtable().HasMethod(this->name_, this->raw_descriptor_)) return false;
  }
  return true;
}

llvm::Function *MethodDeclaration::CreateFunctionInModule(llvm::Module *module, const std::string &mangled_name) const {
  llvm::Function *function =
      llvm::Function::Create(this->function_type_, llvm::GlobalValue::ExternalLinkage, mangled_name, module);
  function->setCallingConv(llvm::CallingConv::C);

  if (this->descriptor_->HasThisParameter()) {
    // "this" pointer will always be non-null (it is the responsibility of the caller to throw NullPointerException).
    function->addParamAttr(0, llvm::Attribute::NonNull);
    function->addParamAttr(0, llvm::Attribute::NoUndef);
    function->getArg(0)->setName("this");
  }

  if (this->descriptor_->return_type() != Type::kVoid) function->addRetAttr(llvm::Attribute::NoUndef);
  for (size_t i = 0; i < function->arg_size(); ++i) { function->addParamAttr(i, llvm::Attribute::NoUndef); }

  return function;
}
llvm::Function *MethodDeclaration::GetFunctionInModule(llvm::Module *module) {
  const auto &it = this->functions_.find(module);
  if (it != this->functions_.end()) return it->second;

  llvm::Function *function = this->CreateFunctionInModule(module, this->mangled_name_);
  this->functions_.emplace(module, function);
  return function;
}
llvm::Function *MethodDeclaration::GetVirtualDispatchInModule(llvm::Module *module) {
  assert(this->IsVirtual());
  const auto &it = this->virtual_dispatches_.find(module);
  if (it != this->virtual_dispatches_.end()) return it->second;

  llvm::Function *function = this->CreateFunctionInModule(module, this->virtual_mangled_name_);
  this->virtual_dispatches_.emplace(module, function);
  return function;
}

void MethodDeclaration::EmitDefinition(llvm::Module *module) {
  assert(this->bytecode_ != nullptr);
  assert(this->owner_ != nullptr);

  // TODO: Implement natives
  if (this->bytecode_->access_flags() & cjbp::AccessFlags::kNative) return;

  llvm::Function *function = this->GetFunctionInModule(module);
  codegen::EmitMethod(this->owner_, this, this->bytecode_, function, module);

  if (this->IsVirtual()) { this->EmitVirtualDispatchThunkDefinition(module); }
}

namespace {
llvm::Value *EmitMethodCall(llvm::IRBuilder<> &builder, llvm::FunctionType *func_type, llvm::Value *function,
                            std::optional<Value> object_ref, const std::vector<Value> &params,
                            const std::string &name) {
  std::vector<llvm::Value *> raw_params{};
  raw_params.reserve((object_ref.has_value() ? 1 : 0) + params.size());
  if (object_ref.has_value()) raw_params.push_back(object_ref->value);
  for (const Value &param : params) { raw_params.push_back(param.value); }
  return builder.CreateCall(func_type, function, raw_params, name);
}
}// namespace
Value MethodDeclaration::EmitCall(llvm::IRBuilder<> &builder, std::optional<Value> object_ref,
                                  const std::vector<Value> &params, const std::string &name) {
  llvm::Function *function = this->GetFunctionInModule(builder.GetInsertBlock()->getModule());
  llvm::Value *call_result = EmitMethodCall(builder, this->function_type_, function, object_ref, params, name);
  return {call_result, this->descriptor_->return_type()};
}
Value MethodDeclaration::EmitVirtualCall(llvm::IRBuilder<> &builder, Value object_ref, const std::vector<Value> &params,
                                         const std::string &name) {
  llvm::Function *function = this->GetVirtualDispatchInModule(builder.GetInsertBlock()->getModule());
  llvm::Value *call_result = EmitMethodCall(builder, this->function_type_, function, object_ref, params, name);
  return {call_result, this->descriptor_->return_type()};
}
void MethodDeclaration::EmitVirtualDispatchThunkDefinition(llvm::Module *module) {
  assert(this->IsVirtual());
  assert(this->owner_ != nullptr);

  llvm::Function *function = this->GetVirtualDispatchInModule(module);
  llvm::BasicBlock *block = llvm::BasicBlock::Create(*this->ctx_->llvm_ctx(), "", function);
  llvm::IRBuilder<> builder(*this->ctx_->llvm_ctx());
  builder.SetInsertPoint(block);

  llvm::Value *method = this->owner_->vtable().EmitVirtualLookup(builder, {function->getArg(0), Type::kObject},
                                                                 this->name_, this->raw_descriptor_);
  std::vector<llvm::Value *> args{};
  args.reserve(function->arg_size());
  for (size_t i = 0; i < function->arg_size(); ++i) { args.push_back(function->getArg(i)); }
  llvm::Value *call_result = builder.CreateCall(this->function_type_, method, args, "");
  if (this->descriptor_->return_type() != Type::kVoid) {
    call_result->setName("call_result");
    builder.CreateRet(call_result);
  } else {
    builder.CreateRetVoid();
  }
}

}// namespace magnetic
