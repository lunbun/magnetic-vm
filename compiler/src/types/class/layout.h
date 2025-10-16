//
// Created by lunbun on 6/10/2022.
//

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <memory>

#include <llvm/IR/DataLayout.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include "types/type.h"

namespace magnetic {

class StructElementLayoutSpecifier {
 public:
  explicit StructElementLayoutSpecifier(llvm::Type *layout_type)
      : element_index_(-1), byte_offset_(-1), layout_type_(layout_type) {}

  [[nodiscard]] ssize_t element_index() const { return this->element_index_; }
  [[nodiscard]] ssize_t byte_offset() const { return this->byte_offset_; }

  void AppendElementType(std::vector<llvm::Type *> &struct_elements);
  void ComputeByteOffset(const llvm::StructLayout *struct_layout);

  [[nodiscard]] llvm::Value *EmitGEP(llvm::IRBuilder<> &builder, llvm::Value *ptr, const std::string &name) const;

 private:
  ssize_t element_index_;
  ssize_t byte_offset_;
  llvm::Type *layout_type_;
};

void SetStructBodyFromLayout(llvm::StructType *struct_type, const llvm::DataLayout &data_layout, const std::vector<StructElementLayoutSpecifier *> &element_layout);

}// namespace magnetic
