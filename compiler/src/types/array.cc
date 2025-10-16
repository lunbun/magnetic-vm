//
// Created by lunbun on 7/16/2022.
//

#include "array.h"

#include "type.h"
#include "context/context.h"

namespace magnetic {

namespace {
class ArrayInfoImpl : public IArrayInfo {
 public:
  Value EmitGetLength(llvm::IRBuilder<> &builder, Value array_ref) const override {
    assert(array_ref.type == Type::kObject);
    builder.CreateLoad(this)
  }

  [[nodiscard]] Type element_type() const override { return this->element_type_; }

 private:
  Context *ctx_;
  Type element_type_;
};
}// namespace

}// namespace magnetic
