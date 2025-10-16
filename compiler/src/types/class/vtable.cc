//
// Created by lunbun on 6/23/2022.
//

#include "vtable.h"

#include <string>

#include "context/context.h"
#include "context/exception.h"
#include "method.h"
#include "types/mangle.h"

namespace magnetic {

namespace {
std::string GetVirtualMethodKey(const std::string &name, const std::string &descriptor) { return name + descriptor; }
std::string GetVirtualMethodKey(MethodDeclaration *method) {
  return GetVirtualMethodKey(method->name(), method->raw_descriptor());
}
}// namespace

VTable VTable::CreateVTableForBaseClass(Context *ctx, const std::string &class_name) { return {ctx, class_name}; }
VTable::VTable(Context *ctx, const std::string &class_name)
    : ctx_(ctx), layout_(ctx->ptr_type()), subclass_(class_name), base_class_(class_name), vtable_(nullptr), methods_(),
      entries_() {}

VTable VTable::CreateVTableForSubClass(const VTable &base_vtable, std::string subclass) {
  return {base_vtable, std::move(subclass)};
}
VTable::VTable(const VTable &base_vtable, std::string subclass)
    : ctx_(base_vtable.ctx_), layout_(base_vtable.layout_), subclass_(std::move(subclass)),
      base_class_(base_vtable.base_class_), vtable_(nullptr), methods_(), entries_() {
  this->entries_.reserve(base_vtable.entries_.size());
  for (const auto &entry : base_vtable.entries_) { this->entries_.push_back(entry->Copy(this)); }
}

void VTable::EmitDefinition(llvm::Module *module) {
  std::vector<llvm::Constant *> values{};
  values.reserve(this->entries_.size());
  for (const auto &entry : this->entries_) { values.push_back(entry->value(module)); }

  llvm::ArrayType *array_type = llvm::ArrayType::get(this->ctx_->ptr_type(), values.size());
  llvm::Constant *const_array = llvm::ConstantArray::get(array_type, values);
  std::string mangled_name = this->ctx_->name_mangler()->MangleVTableName(this->subclass_, this->base_class_);
  this->vtable_ =
      new llvm::GlobalVariable(*module, array_type, true, llvm::GlobalValue::PrivateLinkage, const_array, mangled_name);
}

bool VTable::HasMethod(const std::string &name, const std::string &descriptor) const {
  std::string key = GetVirtualMethodKey(name, descriptor);
  return (this->methods_.find(key) != this->methods_.end());
}
void VTable::MaybeAddVirtualMethod(MethodDeclaration *method) {
  if (!method->IsVirtual()) return;

  std::string key = GetVirtualMethodKey(method);
  const auto &it = this->methods_.find(key);
  if (it != this->methods_.end()) {
    it->second->set_method(method);
  } else {
    int32_t index = this->GetNextEntryIndex();
    auto entry = std::make_unique<VirtualMethodEntry>(index, method);
    this->methods_.emplace(std::move(key), entry.get());
    this->entries_.push_back(std::move(entry));
  }
}

llvm::Value *VTable::EmitVirtualLookup(llvm::IRBuilder<> &builder, Value object_ref, const std::string &name,
                                       const std::string &descriptor) const {
  assert(object_ref.type == Type::kObject);
  assert(object_ref.value != nullptr);

  std::string key = GetVirtualMethodKey(name, descriptor);
  const auto &it = this->methods_.find(key);
  if (it == this->methods_.end()) throw BadBytecode("could not find virtual method " + key);
  VirtualMethodEntry *entry = it->second;

  llvm::Value *vtable_gep = this->layout_.EmitGEP(builder, object_ref.value, "vtable_gep");
  llvm::Value *vtable_ptr = builder.CreateLoad(this->ctx_->ptr_type(), vtable_gep, "vtable_ptr");
  llvm::Value *method_gep =
      builder.CreateConstInBoundsGEP1_32(this->ctx_->ptr_type(), vtable_ptr, entry->index(), "method_gep");
  llvm::Value *method = builder.CreateLoad(this->ctx_->ptr_type(), method_gep, "method");
  return method;
}
void VTable::EmitStoreVTablePointer(llvm::IRBuilder<> &builder, Value object_ref) const {
  assert(this->vtable_ != nullptr);
  assert(object_ref.type == Type::kObject);
  assert(object_ref.value != nullptr);

  llvm::Value *vtable_gep = this->layout_.EmitGEP(builder, object_ref.value, "vtable_gep");
  builder.CreateStore(this->vtable_, vtable_gep);
}

int32_t VTable::GetNextEntryIndex() const { return static_cast<int32_t>(this->entries_.size()); }

VTable::Entry::Entry(int32_t index) : index_(index) {}
VTable::VirtualMethodEntry::VirtualMethodEntry(int32_t index, MethodDeclaration *method)
    : Entry(index), method_(method) {}
llvm::Constant *VTable::VirtualMethodEntry::value(llvm::Module *module) const {
  return this->method_->GetFunctionInModule(module);
}
std::unique_ptr<VTable::Entry> VTable::VirtualMethodEntry::Copy(VTable *new_vtable) const {
  std::string key = GetVirtualMethodKey(this->method_);
  auto new_entry = std::make_unique<VTable::VirtualMethodEntry>(this->index(), this->method_);
  new_vtable->methods_.emplace(std::move(key), new_entry.get());
  return new_entry;
}

}// namespace magnetic
