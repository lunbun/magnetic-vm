//
// Created by lunbun on 6/23/2022.
//

#pragma once

#include <map>
#include <memory>
#include <string>

#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Value.h>

#include "layout.h"

namespace magnetic {

class MethodDeclaration;

class VTable {
 public:
  static VTable CreateVTableForBaseClass(Context *ctx, const std::string &class_name);
  static VTable CreateVTableForSubClass(const VTable &base_vtable, std::string subclass);

  VTable() = delete;
  VTable(const VTable &) = delete;
  VTable &operator=(const VTable &) = delete;
  VTable(VTable &&) = default;
  VTable &operator=(VTable &&) = default;
  VTable(Context *ctx, const std::string &class_name);
  VTable(const VTable &base_vtable, std::string subclass);

  [[nodiscard]] StructElementLayoutSpecifier &layout() { return this->layout_; }

  void EmitDefinition(llvm::Module *module);

  [[nodiscard]] bool HasMethod(const std::string &name, const std::string &descriptor) const;
  void MaybeAddVirtualMethod(MethodDeclaration *method);

  [[nodiscard]] llvm::Value *EmitVirtualLookup(llvm::IRBuilder<> &builder, Value object_ref, const std::string &name,
                                               const std::string &descriptor) const;
  void EmitStoreVTablePointer(llvm::IRBuilder<> &builder, Value object_ref) const;

 private:
  class Entry {
   public:
    explicit Entry(int32_t index);
    Entry(const Entry &) = delete;
    Entry &operator=(const Entry &) = delete;
    Entry(Entry &&) = delete;
    Entry &operator=(Entry &&) = delete;
    virtual ~Entry() noexcept = default;

    [[nodiscard]] int32_t index() const { return this->index_; }
    [[nodiscard]] virtual llvm::Constant *value(llvm::Module *module) const = 0;

    [[nodiscard]] virtual std::unique_ptr<Entry> Copy(VTable *new_vtable) const = 0;

   private:
    int32_t index_;
  };
  class VirtualMethodEntry : public Entry {
   public:
    VirtualMethodEntry(int32_t index, MethodDeclaration *method);
    ~VirtualMethodEntry() noexcept override = default;

    void set_method(MethodDeclaration *method) { this->method_ = method; }
    [[nodiscard]] MethodDeclaration *method() const { return this->method_; }
    [[nodiscard]] llvm::Constant *value(llvm::Module *module) const override;

    [[nodiscard]] std::unique_ptr<Entry> Copy(VTable *new_vtable) const override;

   private:
    MethodDeclaration *method_;
  };

  Context *ctx_;
  StructElementLayoutSpecifier layout_;

  std::string subclass_;
  std::string base_class_;
  llvm::GlobalVariable *vtable_;
  std::map<std::string, VirtualMethodEntry *> methods_;
  std::vector<std::unique_ptr<Entry>> entries_;

  [[nodiscard]] int32_t GetNextEntryIndex() const;
};

}// namespace magnetic
