//
// Created by lunbun on 6/10/2022.
//

#include "field.h"

#include <cassert>
#include <optional>
#include <string>

#include <llvm/IR/GlobalVariable.h>

#include "class/descriptor.h"
#include "compilation-unit/compilation-unit.h"
#include "context/context.h"
#include "layout.h"
#include "types/mangle.h"

namespace magnetic {

FieldDeclaration::FieldDeclaration(Context *ctx, const std::string &descriptor) : ctx_(ctx) {
  this->descriptor_ = ParseTypeDescriptor(ctx, descriptor, false);
}

namespace {
class StaticFieldDeclaration : public FieldDeclaration {
 public:
  StaticFieldDeclaration(Context *ctx, const std::string_view class_name, const std::string_view name,
                         const std::string &descriptor)
      : FieldDeclaration(ctx, descriptor), globals_() {
    this->mangled_name_ = ctx->name_mangler()->MangleStaticFieldName(class_name, name, descriptor);
  }
  ~StaticFieldDeclaration() noexcept override = default;

  void EmitDefinition(llvm::Module *module) override {
    llvm::GlobalVariable *global = this->GetGlobalInModule(module);
    global->setLinkage(llvm::GlobalVariable::CommonLinkage);
  }

  Value EmitLoad(llvm::IRBuilder<> &builder, std::optional<Value> object_ref, const std::string &name) override {
    assert(!object_ref.has_value());
    llvm::GlobalVariable *global = this->GetGlobalInModule(builder.GetInsertBlock()->getModule());
    llvm::Value *value = builder.CreateLoad(this->descriptor().llvm_type(this->ctx()), global, name);
    return {value, this->descriptor()};
  }
  void EmitStore(llvm::IRBuilder<> &builder, std::optional<Value> object_ref, Value value) override {
    assert(!object_ref.has_value());
    llvm::GlobalVariable *global = this->GetGlobalInModule(builder.GetInsertBlock()->getModule());
    builder.CreateStore(value.value, global);
  }

  [[nodiscard]] StructElementLayoutSpecifier *element_layout() override { return nullptr; }

 private:
  std::string mangled_name_;
  std::map<llvm::Module *, llvm::GlobalVariable *> globals_;

  llvm::GlobalVariable *GetGlobalInModule(llvm::Module *module) {
    const auto &it = this->globals_.find(module);
    if (it != this->globals_.end()) return it->second;

    auto global = new llvm::GlobalVariable(*module, this->descriptor().llvm_type(this->ctx()), false,
                                           llvm::GlobalValue::ExternalLinkage, nullptr, this->mangled_name_);
    this->globals_.emplace(module, global);
    return global;
  }
};

class InstanceFieldDeclaration : public FieldDeclaration, public StructElementLayoutSpecifier {
 public:
  InstanceFieldDeclaration(Context *ctx, const std::string_view class_name, const std::string_view name,
                           const std::string &descriptor)
      : FieldDeclaration(ctx, descriptor), StructElementLayoutSpecifier(this->descriptor().llvm_type(this->ctx())),
        getters_(), setters_() {
    this->getter_mangled_name_ = ctx->name_mangler()->MangleInstanceFieldGetter(class_name, name, descriptor);
    this->setter_mangled_name_ = ctx->name_mangler()->MangleInstanceFieldSetter(class_name, name, descriptor);
  }
  ~InstanceFieldDeclaration() noexcept override = default;

  void EmitDefinition(llvm::Module *module) override {
    llvm::IRBuilder<> builder(*this->ctx()->llvm_ctx());
    {
      llvm::Function *getter = this->GetGetterInModule(module);
      llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(*this->ctx()->llvm_ctx(), "", getter);
      builder.SetInsertPoint(entry_block);

      llvm::Value *field_ptr = this->EmitGEP(builder, getter->getArg(0), "field_ptr");
      llvm::Value *field_value =
          builder.CreateLoad(this->descriptor().llvm_type(this->ctx()), field_ptr, "field_value");
      builder.CreateRet(field_value);
    }
    {
      llvm::Function *setter = this->GetSetterInModule(module);
      setter->deleteBody();
      llvm::BasicBlock *entry_block = llvm::BasicBlock::Create(*this->ctx()->llvm_ctx(), "", setter);
      builder.SetInsertPoint(entry_block);

      llvm::Value *field_ptr = this->EmitGEP(builder, setter->getArg(0), "field_ptr");
      builder.CreateStore(setter->getArg(1), field_ptr);
      builder.CreateRetVoid();
    }
  }

  Value EmitLoad(llvm::IRBuilder<> &builder, std::optional<Value> object_ref, const std::string &name) override {
    assert(object_ref.has_value());
    llvm::Function *getter = this->GetGetterInModule(builder.GetInsertBlock()->getModule());
    std::vector<llvm::Value *> params = {object_ref->value};
    llvm::Value *value = builder.CreateCall(getter, params, name);
    return {value, this->descriptor()};
  }
  void EmitStore(llvm::IRBuilder<> &builder, std::optional<Value> object_ref, Value value) override {
    assert(object_ref.has_value());
    llvm::Function *setter = this->GetSetterInModule(builder.GetInsertBlock()->getModule());
    std::vector<llvm::Value *> params = {object_ref->value, value.value};
    builder.CreateCall(setter, params);
  }

  [[nodiscard]] StructElementLayoutSpecifier *element_layout() override { return this; }

 private:
  std::string getter_mangled_name_;
  std::string setter_mangled_name_;
  std::map<llvm::Module *, llvm::Function *> getters_;
  std::map<llvm::Module *, llvm::Function *> setters_;

  llvm::Function *GetGetterInModule(llvm::Module *module) {
    const auto &it = this->getters_.find(module);
    if (it != this->getters_.end()) return it->second;

    std::vector<llvm::Type *> params = {this->ctx()->ptr_type()};
    llvm::FunctionType *func_type = llvm::FunctionType::get(this->descriptor().llvm_type(this->ctx()), params, false);
    llvm::Function *getter =
        llvm::Function::Create(func_type, llvm::GlobalValue::ExternalLinkage, this->getter_mangled_name_, *module);
    getter->getArg(0)->setName("this");
    getter->addRetAttr(llvm::Attribute::NoUndef);
    getter->addParamAttr(0, llvm::Attribute::NoAlias);
    getter->addParamAttr(0, llvm::Attribute::NoCapture);
    getter->addParamAttr(0, llvm::Attribute::NonNull);
    getter->addParamAttr(0, llvm::Attribute::NoUndef);
    getter->addParamAttr(0, llvm::Attribute::ReadOnly);
    getter->addFnAttr(llvm::Attribute::ArgMemOnly);
    getter->addFnAttr(llvm::Attribute::AlwaysInline);
    getter->addFnAttr(llvm::Attribute::MustProgress);
    getter->addFnAttr(llvm::Attribute::NoFree);
    getter->addFnAttr(llvm::Attribute::NoRecurse);
    getter->addFnAttr(llvm::Attribute::NoSync);
    getter->addFnAttr(llvm::Attribute::NoUnwind);
    getter->addFnAttr(llvm::Attribute::WillReturn);
    getter->addFnAttr(llvm::Attribute::ReadOnly);

    this->getters_.emplace(module, getter);
    return getter;
  }

  llvm::Function *GetSetterInModule(llvm::Module *module) {
    const auto &it = this->setters_.find(module);
    if (it != this->setters_.end()) return it->second;

    std::vector<llvm::Type *> params = {this->ctx()->ptr_type(), this->descriptor().llvm_type(this->ctx())};
    llvm::FunctionType *func_type = llvm::FunctionType::get(this->ctx()->void_type(), params, false);
    llvm::Function *setter =
        llvm::Function::Create(func_type, llvm::GlobalValue::ExternalLinkage, this->setter_mangled_name_, module);
    setter->getArg(0)->setName("this");
    setter->getArg(1)->setName("value");
    setter->addParamAttr(0, llvm::Attribute::NoCapture);
    setter->addParamAttr(0, llvm::Attribute::NonNull);
    setter->addParamAttr(0, llvm::Attribute::NoUndef);
    setter->addParamAttr(0, llvm::Attribute::WriteOnly);
    setter->addParamAttr(1, llvm::Attribute::NoUndef);
    setter->addFnAttr(llvm::Attribute::ArgMemOnly);
    setter->addFnAttr(llvm::Attribute::AlwaysInline);
    setter->addFnAttr(llvm::Attribute::MustProgress);
    setter->addFnAttr(llvm::Attribute::NoFree);
    setter->addFnAttr(llvm::Attribute::NoRecurse);
    setter->addFnAttr(llvm::Attribute::NoSync);
    setter->addFnAttr(llvm::Attribute::NoUnwind);
    setter->addFnAttr(llvm::Attribute::WillReturn);
    setter->addFnAttr(llvm::Attribute::WriteOnly);

    this->setters_.emplace(module, setter);
    return setter;
  }
};
}// namespace

std::unique_ptr<FieldDeclaration> FieldDeclaration::Create(Context *ctx, bool is_static, std::string_view class_name,
                                                           std::string_view name, const std::string &descriptor) {
  if (is_static) {
    return std::make_unique<StaticFieldDeclaration>(ctx, class_name, name, descriptor);
  } else {
    return std::make_unique<InstanceFieldDeclaration>(ctx, class_name, name, descriptor);
  }
}

}// namespace magnetic
