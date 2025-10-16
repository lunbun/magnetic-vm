//
// Created by lunbun on 6/9/2022.
//

#include "type.h"

#include <cassert>

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Type.h>

#include "context/context.h"

namespace magnetic {

int32_t Type::width() const {
  switch (this->kind_) {
    case Kind::kVoid: return 0;
    case Kind::kInt:
    case Kind::kFloat:
    case Kind::kObject: return 1;
    case Kind::kLong:
    case Kind::kDouble: return 2;
    default: return 0;
  }
}
const char *Type::name() const {
  switch (this->kind_) {
    case Kind::kInt: return "int";
    case Kind::kLong: return "long";
    case Kind::kFloat: return "float";
    case Kind::kDouble: return "double";
    case Kind::kObject: return "object";
    case Kind::kVoid: return "void";
    default: return "unknown";
  }
}
llvm::Type *Type::llvm_type(Context *ctx) const {
  switch (this->kind_) {
    case Kind::kInt: return ctx->int32();
    case Kind::kLong: return ctx->int64();
    case Kind::kFloat: return ctx->float32();
    case Kind::kDouble: return ctx->float64();
    case Kind::kObject: return ctx->ptr_type();
    case Kind::kVoid: return ctx->void_type();
    default: return nullptr;
  }
}

}// namespace magnetic
