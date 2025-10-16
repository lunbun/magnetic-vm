//
// Created by lunbun on 7/9/2022.
//

#pragma once

#include <memory>
#include <string>

namespace magnetic {

class NameMangler {
 public:
  static std::unique_ptr<NameMangler> CreateSimpleMangler();
  static std::unique_ptr<NameMangler> CreateJNIMangler();

  NameMangler(const NameMangler &) = delete;
  NameMangler &operator=(const NameMangler &) = delete;
  NameMangler(NameMangler &&) = delete;
  NameMangler &operator=(NameMangler &&) = delete;
  virtual ~NameMangler() noexcept = default;

  [[nodiscard]] virtual std::string MangleFullyQualifiedClassName(std::string_view class_name) const = 0;
  [[nodiscard]] virtual std::string MangleMethodName(std::string_view class_name, std::string_view name,
                                                     std::string_view descriptor) const = 0;
  [[nodiscard]] virtual std::string MangleVirtualDispatchName(std::string_view class_name, std::string_view name,
                                                              std::string_view descriptor) const = 0;
  [[nodiscard]] virtual std::string MangleVTableName(std::string_view subclass, std::string_view base_class) const = 0;
  [[nodiscard]] virtual std::string MangleStaticFieldName(std::string_view class_name, std::string_view name,
                                                          std::string_view descriptor) const = 0;
  [[nodiscard]] virtual std::string MangleInstanceFieldGetter(std::string_view class_name, std::string_view name,
                                                              std::string_view descriptor) const = 0;
  [[nodiscard]] virtual std::string MangleInstanceFieldSetter(std::string_view class_name, std::string_view name,
                                                              std::string_view descriptor) const = 0;
  [[nodiscard]] virtual std::string MangleInstantiatorName(std::string_view class_name) const = 0;

 protected:
  NameMangler() = default;
};

}// namespace magnetic
