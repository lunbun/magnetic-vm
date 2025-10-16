//
// Created by lunbun on 7/7/2022.
//

#include "instantiate.h"

#include "class.h"
#include "context/context.h"
#include "types/mangle.h"

namespace magnetic {

std::unique_ptr<ClassInstantiator> ClassInstantiator::Create(Context *ctx, std::string class_name) {
  return std::make_unique<ClassInstantiator>(ctx, std::move(class_name));
}
ClassInstantiator::ClassInstantiator(Context *ctx, std::string class_name)
    : ctx_(ctx), class_name_(std::move(class_name)), owner_(nullptr), instantiators_() {
  this->mangled_name_ = ctx->name_mangler()->MangleInstantiatorName(this->class_name_);
}

void ClassInstantiator::EmitDefinition(llvm::Module *module) {
  assert(this->owner_ != nullptr);

  llvm::Function *function = this->GetInstantiatorInModule(module);
  llvm::BasicBlock *block = llvm::BasicBlock::Create(*this->ctx_->llvm_ctx(), "", function);
  llvm::IRBuilder<> builder(*this->ctx_->llvm_ctx());
  builder.SetInsertPoint(block);

  llvm::IntegerType *int_type = builder.getInt32Ty();
  llvm::Type *type = this->owner_->struct_type();
  llvm::Constant *malloc_size = llvm::ConstantInt::get(int_type, module->getDataLayout().getTypeAllocSize(type), true);
  llvm::Value *ptr =
      llvm::CallInst::CreateMalloc(builder.GetInsertBlock(), int_type, type, malloc_size, nullptr, nullptr, "");
  builder.Insert(ptr);

  llvm::Align align = module->getDataLayout().getABITypeAlign(type);
  builder.CreateMemSet(ptr, llvm::ConstantInt::get(builder.getInt8Ty(), 0, true), malloc_size, align);

  this->owner_->vtable().EmitStoreVTablePointer(builder, {ptr, Type::kObject});

  builder.CreateRet(ptr);
}
Value ClassInstantiator::EmitInstantiation(llvm::IRBuilder<> &builder, const std::string &name) {
  llvm::Function *function = this->GetInstantiatorInModule(builder.GetInsertBlock()->getModule());
  llvm::CallInst *instance = builder.CreateCall(function, llvm::None, name);
  return {instance, Type::kObject};
}

llvm::Function *ClassInstantiator::GetInstantiatorInModule(llvm::Module *module) {
  const auto &it = this->instantiators_.find(module);
  if (it != this->instantiators_.end()) return it->second;

  llvm::FunctionType *function_type = llvm::FunctionType::get(this->ctx_->ptr_type(), llvm::None, false);
  llvm::Function *function =
      llvm::Function::Create(function_type, llvm::GlobalValue::ExternalLinkage, this->mangled_name_, *module);
  function->addRetAttr(llvm::Attribute::NoAlias);
  function->addRetAttr(llvm::Attribute::NoUndef);
  function->addFnAttr(llvm::Attribute::AlwaysInline);
  function->addFnAttr(llvm::Attribute::InaccessibleMemOnly);
  function->addFnAttr(llvm::Attribute::MustProgress);
  function->addFnAttr(llvm::Attribute::NoFree);
  function->addFnAttr(llvm::Attribute::NoRecurse);
  function->addFnAttr(llvm::Attribute::NoSync);
  function->addFnAttr(llvm::Attribute::NoUnwind);
  function->addFnAttr(llvm::Attribute::WillReturn);

  this->instantiators_.emplace(module, function);
  return function;
}

}// namespace magnetic
