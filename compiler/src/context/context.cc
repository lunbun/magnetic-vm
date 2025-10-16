//
// Created by lunbun on 6/9/2022.
//

#include "context.h"

#include <memory>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>

#include "class/class.h"
#include "class/mangle.h"
#include "class/pool/pool.h"
#include "codegen/runtime-abi.h"
#include "compilation-unit/compilation-unit.h"
#include "types/type.h"

namespace magnetic {

Context::Context() : fields_(), methods_(), instantiators_(), single_unit_compilation_(false), global_unit_(nullptr) {
  this->ctx_ = std::make_unique<llvm::LLVMContext>();
  this->ctx_->enableOpaquePointers();

  this->i1_ = llvm::Type::getInt1Ty(*this->ctx_);
  this->i8_ = llvm::Type::getInt8Ty(*this->ctx_);
  this->i16_ = llvm::Type::getInt16Ty(*this->ctx_);
  this->i32_ = llvm::Type::getInt32Ty(*this->ctx_);
  this->i64_ = llvm::Type::getInt64Ty(*this->ctx_);
  this->f32_ = llvm::Type::getFloatTy(*this->ctx_);
  this->f64_ = llvm::Type::getDoubleTy(*this->ctx_);
  this->void_type_ = llvm::Type::getVoidTy(*this->ctx_);
  this->ptr_type_ = llvm::PointerType::get(*this->ctx_, 0);
  this->pointer_null_ = llvm::ConstantPointerNull::get(this->ptr_type_);
}
Context::~Context() noexcept = default;

void Context::set_pool(std::unique_ptr<ClassPool> pool) {
  pool->set_ctx(this);
  this->pool_ = std::move(pool);
}

namespace {
std::string GetFieldKey(const std::string &class_name, const std::string &name, const std::string &descriptor) {
  return class_name + ' ' + name + ' ' + descriptor;
}
std::string GetMethodKey(const std::string &class_name, const std::string &name, const std::string &descriptor) {
  return class_name + ' ' + name + ' ' + descriptor;
}
}// namespace
FieldDeclaration *Context::GetField(const std::string &class_name, const std::string &name,
                                    const std::string &descriptor, bool is_static) {
  std::string key = GetFieldKey(class_name, name, descriptor);
  const auto &it = this->fields_.find(key);
  if (it != this->fields_.end()) return it->second.get();

  this->fields_.emplace(key, FieldDeclaration::Create(this, is_static, class_name, name, descriptor));
  return this->fields_.at(key).get();
}
MethodDeclaration *Context::GetMethod(const std::string &class_name, const std::string &name,
                                      const std::string &descriptor, bool is_static) {
  std::string key = GetMethodKey(class_name, name, descriptor);
  const auto &it = this->methods_.find(key);
  if (it != this->methods_.end()) return it->second.get();

  this->methods_.emplace(key, MethodDeclaration::Create(this, is_static, class_name, name, descriptor));
  return this->methods_.at(key).get();
}
ClassInstantiator *Context::GetInstantiator(const std::string &class_name) {
  const auto &it = this->instantiators_.find(class_name);
  if (it != this->instantiators_.end()) return it->second.get();

  this->instantiators_.emplace(class_name, ClassInstantiator::Create(this, class_name));
  return this->instantiators_.at(class_name).get();
}

void Context::set_name_mangler(std::unique_ptr<NameMangler> name_mangler) {
  this->name_mangler_ = std::move(name_mangler);
}

void Context::set_runtime_abi(std::unique_ptr<RuntimeABI> runtime_abi) {
  runtime_abi->set_ctx(this);
  this->runtime_abi_ = std::move(runtime_abi);
}

std::shared_ptr<CompilationUnit> Context::CreateCompilationUnitForClass(const std::string &class_name) {
  if (this->single_unit_compilation_) {
    if (this->global_unit_ == nullptr) { this->global_unit_ = std::make_shared<CompilationUnit>("global_unit", this); }
    return this->global_unit_;
  } else {
    return std::make_shared<CompilationUnit>(class_name, this);
  }
}

}// namespace magnetic
