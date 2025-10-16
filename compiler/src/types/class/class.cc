//
// Created by lunbun on 6/9/2022.
//

#include "class.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <optional>

#include <cjbp/cjbp.h>
#include <fmt/core.h>
#include <llvm/IR/Module.h>

#include "compilation-unit/compilation-unit.h"
#include "context/context.h"
#include "context/exception.h"
#include "field.h"
#include "instantiate.h"
#include "method.h"
#include "types/mangle.h"
#include "types/pool/pool.h"

namespace magnetic {

ClassInfo::ClassInfo(Context *ctx, std::unique_ptr<cjbp::Class> bytecode,
                     std::shared_ptr<CompilationUnit> compilation_unit)
    : ctx_(ctx), bytecode_(std::move(bytecode)), struct_type_(nullptr), super_class_(nullptr), vtable_(std::nullopt),
      super_class_layout_(std::nullopt) {
  this->struct_type_ = llvm::StructType::create(*this->ctx_->llvm_ctx(), this->name());
  this->compilation_unit_ = std::move(compilation_unit);
}
ClassInfo::~ClassInfo() noexcept = default;
const std::string &ClassInfo::name() const { return this->bytecode_->name(); }
const VTable &ClassInfo::vtable() const {
  assert(this->vtable_.has_value());
  return this->vtable_.value();
}
bool ClassInfo::is_final() const { return (this->bytecode_->access_flags() & cjbp::AccessFlags::kFinal); }

void ClassInfo::EmitDefinition() {
  std::vector<StructElementLayoutSpecifier *> element_layout{};
  std::vector<FieldDeclaration *> owned_fields{};
  owned_fields.reserve(this->bytecode_->fields().size());
  std::vector<MethodDeclaration *> owned_methods{};
  owned_methods.reserve(this->bytecode_->methods().size());

  if (this->bytecode_->super_class() == nullptr) {
    // java.lang.Object doesn't have a super class.
    this->super_class_ = nullptr;
    this->super_class_layout_ = std::nullopt;
    this->vtable_ = VTable::CreateVTableForBaseClass(this->ctx_, this->name());
    element_layout.push_back(&this->vtable_->layout());
  } else {
    this->super_class_ = this->ctx_->pool()->Get(*this->bytecode_->super_class());
    if (this->super_class_ == nullptr) {
      throw BadBytecode(
          fmt::format("could not find {}'s super class {}", this->name(), *this->bytecode_->super_class()));
    }
    this->super_class_layout_ = StructElementLayoutSpecifier(this->super_class_->struct_type_);
    this->vtable_ = VTable::CreateVTableForSubClass(this->super_class_->vtable_.value(), this->name());
    element_layout.push_back(&this->super_class_layout_.value());
  }

  for (const auto &field_bytecode : this->bytecode_->fields()) {
    bool is_static = (field_bytecode->access_flags() & cjbp::AccessFlags::kStatic);
    FieldDeclaration *field =
        this->ctx_->GetField(this->name(), field_bytecode->name(), field_bytecode->descriptor(), is_static);
    if (field->element_layout() != nullptr) element_layout.push_back(field->element_layout());
    owned_fields.push_back(field);
  }

  SetStructBodyFromLayout(this->struct_type_, this->compilation_unit_->module()->getDataLayout(), element_layout);

  for (const auto &method_bytecode : this->bytecode_->methods()) {
    bool is_static = (method_bytecode->access_flags() & cjbp::AccessFlags::kStatic);
    MethodDeclaration *method =
        this->ctx_->GetMethod(this->name(), method_bytecode->name(), method_bytecode->descriptor(), is_static);
    method->set_bytecode(method_bytecode.get());
    method->set_owner(this);
    owned_methods.push_back(method);
    this->vtable_->MaybeAddVirtualMethod(method);
  }

  llvm::Module *module = this->compilation_unit_->module();
  this->vtable_->EmitDefinition(module);
  for (FieldDeclaration *field : owned_fields) { field->EmitDefinition(module); }
  for (MethodDeclaration *method : owned_methods) { method->EmitDefinition(module); }

  ClassInstantiator *instantiator = this->ctx_->GetInstantiator(this->name());
  instantiator->set_owner(this);
  instantiator->EmitDefinition(module);
}

bool ClassInfo::IsSubClassOf(const ClassInfo *other) const {
  if (!this->super_class_layout_.has_value()) return false;
  if (this->super_class_ == other) return true;
  return this->super_class_->IsSubClassOf(other);
}

std::optional<ssize_t> ClassInfo::GetCastOffset(const ClassInfo *dest) const {
  // If the source is a super class of the dest, it's a downcast. We can calculate the offset of a downcast by
  // calculating the offset of an upcast from the dest to the source, and negating the offset.
  bool is_downcast = dest->IsSubClassOf(this);
  const ClassInfo *current_class = this;
  if (is_downcast) { std::swap(current_class, dest); }

  ssize_t offset = 0;
  while (current_class != dest) {
    if (current_class->super_class_ == nullptr) return std::nullopt;

    offset += static_cast<ssize_t>(current_class->super_class_layout_->byte_offset());
    current_class = current_class->super_class_;
  }
  return is_downcast ? -offset : offset;
}
Value ClassInfo::EmitUncheckedClassCastTo(llvm::IRBuilder<> &builder, const ClassInfo *dest, llvm::Value *ptr) const {
  if (this == dest) return {ptr, Type::kObject};

  ssize_t offset = this->GetCastOffset(dest).value();
  if (offset == 0) {
    // If the offset is 0, the GEP will be a no-op, so the entire cast is a no-op.
    return {ptr, Type::kObject};
  }

  llvm::BasicBlock *previous_block = builder.GetInsertBlock();
  llvm::BasicBlock *cast_block =
      llvm::BasicBlock::Create(*this->ctx_->llvm_ctx(), "nonnull_cast", previous_block->getParent());
  llvm::BasicBlock *complete_block =
      llvm::BasicBlock::Create(*this->ctx_->llvm_ctx(), "after_cast", previous_block->getParent());

  // No cast occurs if the pointer is null.
  llvm::Value *is_null = builder.CreateICmpEQ(ptr, this->ctx_->pointer_null(), "cast_is_null");
  builder.CreateCondBr(is_null, complete_block, cast_block);

  builder.SetInsertPoint(cast_block);
  llvm::Value *casted_ptr =
      builder.CreateInBoundsGEP(builder.getInt8Ty(), ptr, llvm::ConstantInt::get(builder.getInt64Ty(), offset));
  builder.CreateBr(complete_block);

  builder.SetInsertPoint(complete_block);
  llvm::PHINode *cast_result = builder.CreatePHI(this->ctx_->ptr_type(), 2, "cast_result");
  cast_result->addIncoming(this->ctx_->pointer_null(), previous_block);
  cast_result->addIncoming(casted_ptr, cast_block);
  return {cast_result, Type::kObject};
}

}// namespace magnetic
