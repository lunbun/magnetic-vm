//
// Created by lunbun on 6/9/2022.
//

#pragma once

#include <cstdint>
#include <string>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

namespace magnetic {

class Context;

class Type {
 public:
  enum Kind : uint8_t { kInt, kLong, kFloat, kDouble, kObject, kVoid };

  Type() = default;
  constexpr /*implicit*/ Type(Kind kind) : kind_(kind) {}// NOLINT(google-explicit-constructor)

  constexpr /*implicit*/ operator Kind() const { return this->kind_; }// NOLINT(google-explicit-constructor)
  explicit operator bool() const = delete;

  friend constexpr bool operator==(Type lhs, Type rhs) { return lhs.kind_ == rhs.kind_; }
  friend constexpr bool operator!=(Type lhs, Type rhs) { return lhs.kind_ != rhs.kind_; }
  friend constexpr bool operator==(Type lhs, Kind rhs) { return lhs.kind_ == rhs; }
  friend constexpr bool operator!=(Type lhs, Kind rhs) { return lhs.kind_ != rhs; }
  friend constexpr bool operator==(Kind lhs, Type rhs) { return lhs == rhs.kind_; }
  friend constexpr bool operator!=(Kind lhs, Type rhs) { return lhs != rhs.kind_; }

  [[nodiscard]] int32_t width() const;
  [[nodiscard]] const char *name() const;
  [[nodiscard]] llvm::Type *llvm_type(Context *ctx) const;

 private:
  Kind kind_;
};

struct Value {
  llvm::Value *value;
  Type type;

  Value(llvm::Value *value, Type type) : value(value), type(type) {}
};

}// namespace magnetic
