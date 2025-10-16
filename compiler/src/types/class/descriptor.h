//
// Created by lunbun on 6/10/2022.
//

#pragma once

#include <vector>

#include <cjbp/cjbp.h>
#include <llvm/IR/DerivedTypes.h>

#include "types/type.h"

namespace magnetic {

class Context;

class MethodDescriptor {
 public:
  static std::unique_ptr<MethodDescriptor> CreateStaticDescriptor(std::vector<Type> params, Type return_type);
  static std::unique_ptr<MethodDescriptor> CreateInstanceDescriptor(std::vector<Type> params, Type return_type);
  static std::unique_ptr<MethodDescriptor> CreateDescriptor(bool is_static, std::vector<Type> params, Type return_type);

  virtual ~MethodDescriptor() noexcept = default;

  [[nodiscard]] virtual bool is_static() const = 0;
  [[nodiscard]] bool HasThisParameter() const;
  [[nodiscard]] virtual const std::vector<Type> &descriptor_params() const = 0;
  [[nodiscard]] virtual const std::vector<Type> &instance_params() const = 0;
  [[nodiscard]] Type return_type() const { return this->return_type_; }

  [[nodiscard]] llvm::FunctionType *CreateFunctionType(Context *ctx) const;

 protected:
  explicit MethodDescriptor(Type return_type) : return_type_(return_type) {}

 private:
  Type return_type_;
};

Type ParseTypeDescriptor(Context *ctx, const std::string &descriptor, bool should_load_classes);
std::unique_ptr<MethodDescriptor> ParseMethodDescriptor(Context *ctx, bool is_static, const std::string &descriptor,
                                                        bool should_load_classes);

}// namespace magnetic
