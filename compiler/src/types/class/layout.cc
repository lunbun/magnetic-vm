//
// Created by lunbun on 6/10/2022.
//

#include "layout.h"

#include <cassert>

namespace magnetic {

void StructElementLayoutSpecifier::AppendElementType(std::vector<llvm::Type *> &struct_elements) {
  this->element_index_ = static_cast<ssize_t>(struct_elements.size());
  struct_elements.push_back(this->layout_type_);
}
void StructElementLayoutSpecifier::ComputeByteOffset(const llvm::StructLayout *struct_layout) {
  this->byte_offset_ = static_cast<ssize_t>(struct_layout->getElementOffset(this->element_index_));
}

llvm::Value *StructElementLayoutSpecifier::EmitGEP(llvm::IRBuilder<> &builder, llvm::Value *ptr,
                                                   const std::string &name) const {
  if (this->byte_offset_ == 0) return ptr;
  // TODO: use CreateStructGEP instead of CreateConstInBoundsGEP1_64
  return builder.CreateConstInBoundsGEP1_64(builder.getInt8Ty(), ptr, this->byte_offset_, "");
}

}// namespace magnetic

void magnetic::SetStructBodyFromLayout(llvm::StructType *struct_type, const llvm::DataLayout &data_layout,
                                       const std::vector<StructElementLayoutSpecifier *> &element_layout) {
  std::vector<llvm::Type *> struct_element_types{};
  struct_element_types.reserve(element_layout.size());
  for (StructElementLayoutSpecifier *element : element_layout) { element->AppendElementType(struct_element_types); }
  struct_type->setBody(struct_element_types, false);
  std::string s = struct_type->getName().str();
  const llvm::StructLayout *struct_layout = data_layout.getStructLayout(struct_type);
  for (StructElementLayoutSpecifier *element : element_layout) { element->ComputeByteOffset(struct_layout); }
}
