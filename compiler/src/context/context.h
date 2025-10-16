//
// Created by lunbun on 6/9/2022.
//

#pragma once

#include <memory>

#include <llvm/IR/LLVMContext.h>

#include "class/field.h"
#include "class/instantiate.h"
#include "class/method.h"
#include "types/type.h"

namespace magnetic {

class Type;
class ClassPool;
class CompilationUnit;
class NameMangler;
class RuntimeABI;

class Context {
 public:
  Context();
  ~Context() noexcept;

  [[nodiscard]] llvm::LLVMContext *llvm_ctx() const { return this->ctx_.get(); }
  [[nodiscard]] ClassPool *pool() const { return this->pool_.get(); }
  void set_pool(std::unique_ptr<ClassPool> pool);

  [[nodiscard]] llvm::IntegerType *int1() const { return this->i1_; }
  [[nodiscard]] llvm::IntegerType *int8() const { return this->i8_; }
  [[nodiscard]] llvm::IntegerType *int16() const { return this->i16_; }
  [[nodiscard]] llvm::IntegerType *int32() const { return this->i32_; }
  [[nodiscard]] llvm::IntegerType *int64() const { return this->i64_; }
  [[nodiscard]] llvm::Type *float32() const { return this->f32_; }
  [[nodiscard]] llvm::Type *float64() const { return this->f64_; }
  [[nodiscard]] llvm::Type *void_type() const { return this->void_type_; }
  [[nodiscard]] llvm::PointerType *ptr_type() const { return this->ptr_type_; }
  [[nodiscard]] llvm::ConstantPointerNull *pointer_null() const { return this->pointer_null_; }

  [[nodiscard]] FieldDeclaration *GetField(const std::string &class_name, const std::string &name,
                                           const std::string &descriptor, bool is_static);
  [[nodiscard]] MethodDeclaration *GetMethod(const std::string &class_name, const std::string &name,
                                             const std::string &descriptor, bool is_static);
  [[nodiscard]] ClassInstantiator *GetInstantiator(const std::string &class_name);

  void set_name_mangler(std::unique_ptr<NameMangler> name_mangler);
  [[nodiscard]] NameMangler *name_mangler() const { return this->name_mangler_.get(); }

  void set_runtime_abi(std::unique_ptr<RuntimeABI> runtime_abi);
  [[nodiscard]] RuntimeABI *runtime_abi() const { return this->runtime_abi_.get(); }

  void set_use_single_unit(bool value) { this->single_unit_compilation_ = value; }
  [[nodiscard]] CompilationUnit *global_unit() const { return this->global_unit_.get(); }
  [[nodiscard]] std::shared_ptr<CompilationUnit> CreateCompilationUnitForClass(const std::string &class_name);

 private:
  std::unique_ptr<llvm::LLVMContext> ctx_;
  std::unique_ptr<ClassPool> pool_;

  llvm::IntegerType *i1_;
  llvm::IntegerType *i8_;
  llvm::IntegerType *i16_;
  llvm::IntegerType *i32_;
  llvm::IntegerType *i64_;
  llvm::Type *f32_;
  llvm::Type *f64_;
  llvm::Type *void_type_;
  llvm::PointerType *ptr_type_;
  llvm::ConstantPointerNull *pointer_null_;

  std::unordered_map<std::string, std::unique_ptr<FieldDeclaration>> fields_;
  std::unordered_map<std::string, std::unique_ptr<MethodDeclaration>> methods_;
  std::unordered_map<std::string, std::unique_ptr<ClassInstantiator>> instantiators_;

  std::unique_ptr<NameMangler> name_mangler_;
  std::unique_ptr<RuntimeABI> runtime_abi_;

  /**
   * All classes are compiled into the same compilation unit. The default behavior (i.e. if this is false) is to give
   * each class its own compilation unit.
   */
  bool single_unit_compilation_;
  std::shared_ptr<CompilationUnit> global_unit_;
};

}// namespace magnetic
