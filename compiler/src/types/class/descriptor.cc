//
// Created by lunbun on 6/10/2022.
//

#include "descriptor.h"

#include <algorithm>
#include <sstream>
#include <vector>

#include <cjbp/cjbp.h>
#include <fmt/core.h>
#include <llvm/IR/DerivedTypes.h>

#include "context/context.h"
#include "types/type.h"

namespace magnetic {
namespace {

class StaticMethodDescriptor : public MethodDescriptor {
 public:
  StaticMethodDescriptor(std::vector<Type> params, Type return_type)
      : MethodDescriptor(return_type), params_(std::move(params)) {}

  [[nodiscard]] bool is_static() const override { return true; }
  [[nodiscard]] const std::vector<Type> &descriptor_params() const override { return this->params_; }
  [[nodiscard]] const std::vector<Type> &instance_params() const override { return this->params_; }

 private:
  std::vector<Type> params_;
};

class InstanceMethodDescriptor : public MethodDescriptor {
 public:
  InstanceMethodDescriptor(std::vector<Type> params, Type return_type)
      : MethodDescriptor(return_type), descriptor_params_(std::move(params)), instance_params_() {
    this->instance_params_ = this->descriptor_params_;
    // Local variable type of "this" pointer is object.
    this->instance_params_.insert(this->instance_params_.begin(), Type::kObject);
  }

  [[nodiscard]] bool is_static() const override { return false; }
  [[nodiscard]] const std::vector<Type> &descriptor_params() const override { return this->descriptor_params_; }
  [[nodiscard]] const std::vector<Type> &instance_params() const override { return this->instance_params_; }

 private:
  std::vector<Type> descriptor_params_;
  std::vector<Type> instance_params_;
};

}// namespace

std::unique_ptr<MethodDescriptor> MethodDescriptor::CreateStaticDescriptor(std::vector<Type> params, Type return_type) {
  return std::make_unique<StaticMethodDescriptor>(std::move(params), return_type);
}
std::unique_ptr<MethodDescriptor> MethodDescriptor::CreateInstanceDescriptor(std::vector<Type> params,
                                                                             Type return_type) {
  return std::make_unique<InstanceMethodDescriptor>(std::move(params), return_type);
}
std::unique_ptr<MethodDescriptor> MethodDescriptor::CreateDescriptor(bool is_static, std::vector<Type> params,
                                                                     Type return_type) {
  return is_static ? CreateStaticDescriptor(std::move(params), return_type)
                   : CreateInstanceDescriptor(std::move(params), return_type);
}

bool MethodDescriptor::HasThisParameter() const { return !this->is_static(); }
llvm::FunctionType *MethodDescriptor::CreateFunctionType(Context *ctx) const {
  std::vector<llvm::Type *> function_params{};
  for (Type param : this->instance_params()) { function_params.push_back(param.llvm_type(ctx)); }
  return llvm::FunctionType::get(this->return_type().llvm_type(ctx), function_params, false);
}
}// namespace magnetic

namespace {
std::string ParseClassNameFromStream(std::stringstream &stream) {
  char c;
  std::stringstream ss{};
  while (stream.peek() != ';') {
    stream.get(c);
    ss << (c == '/' ? '.' : c);
  }
  stream.get();// Skip the ';'
  return stream.str();
}
magnetic::Type ParseTypeFromStream(magnetic::Context *ctx, std::stringstream &stream, bool should_load_classes) {
  char c;
  stream.get(c);
  switch (c) {
    case 'Z':
    case 'I': return magnetic::Type::kInt;
    case 'J': return magnetic::Type::kLong;
    case 'F': return magnetic::Type::kFloat;
    case 'D': return magnetic::Type::kDouble;
    case 'V': return magnetic::Type::kVoid;
    case 'L': {
      std::string class_name = ParseClassNameFromStream(stream);
      if (should_load_classes) {
        throw std::runtime_error("loading descriptor classes not supported");
      } else {
        return magnetic::Type::kObject;
      }
    }
    case '[': {
      magnetic::Type element_type = ParseTypeFromStream(ctx, stream, should_load_classes);
      if (should_load_classes) {
        throw std::runtime_error("loading array classes not supported");
      } else {
        return magnetic::Type::kObject;
      }
    }
    default: throw std::runtime_error(fmt::format("unknown descriptor char '{}'", c));
  }
}
}// namespace
magnetic::Type magnetic::ParseTypeDescriptor(Context *ctx, const std::string &descriptor, bool should_load_classes) {
  std::stringstream ss(descriptor);
  return ParseTypeFromStream(ctx, ss, should_load_classes);
}
std::unique_ptr<magnetic::MethodDescriptor>
magnetic::ParseMethodDescriptor(Context *ctx, bool is_static, const std::string &descriptor, bool should_load_classes) {
  std::stringstream ss(descriptor);
  ss.get();// Descriptor starts with a '('

  std::vector<Type> params{};
  while (ss.peek() != ')') { params.push_back(ParseTypeFromStream(ctx, ss, should_load_classes)); }

  ss.get();// Skip the ')'
  Type return_type = ParseTypeFromStream(ctx, ss, should_load_classes);

  return magnetic::MethodDescriptor::CreateDescriptor(is_static, std::move(params), return_type);
}
