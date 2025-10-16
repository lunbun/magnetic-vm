//
// Created by lunbun on 7/9/2022.
//

#include "mangle.h"

#include <algorithm>
#include <cctype>

#include <fmt/core.h>

namespace magnetic {

namespace {
class SimpleNameMangler : public NameMangler {
 public:
  [[nodiscard]] std::string MangleFullyQualifiedClassName(const std::string_view class_name) const override {
    return std::string(class_name);
  }
  [[nodiscard]] std::string MangleMethodName(const std::string_view class_name, const std::string_view name,
                                             const std::string_view descriptor) const override {
    return MangleReference(class_name, name, descriptor);
  }
  [[nodiscard]] std::string MangleVirtualDispatchName(std::string_view class_name, std::string_view name,
                                                      std::string_view descriptor) const override {
    return "virtual@@" + MangleReference(class_name, name, descriptor);
  }
  [[nodiscard]] std::string MangleVTableName(std::string_view subclass, std::string_view base_class) const override {
    std::string mangled_name("vtable@@");
    mangled_name.append(subclass);
    mangled_name += '@';
    mangled_name.append(base_class);
    return mangled_name;
  }
  [[nodiscard]] std::string MangleStaticFieldName(const std::string_view class_name, const std::string_view name,
                                                  const std::string_view descriptor) const override {
    return MangleReference(class_name, name, descriptor);
  }
  [[nodiscard]] std::string MangleInstanceFieldGetter(const std::string_view class_name, const std::string_view name,
                                                      const std::string_view descriptor) const override {
    return "get@@" + MangleReference(class_name, name, descriptor);
  }
  [[nodiscard]] std::string MangleInstanceFieldSetter(const std::string_view class_name, const std::string_view name,
                                                      const std::string_view descriptor) const override {
    return "set@@" + MangleReference(class_name, name, descriptor);
  }
  [[nodiscard]] std::string MangleInstantiatorName(const std::string_view class_name) const override {
    return "new@@" + this->MangleFullyQualifiedClassName(class_name);
  }

 private:
  static std::string MangleReference(const std::string_view class_name, const std::string_view name,
                                     const std::string_view descriptor) {
    std::string mangled_name(class_name);
    mangled_name += '#';
    mangled_name.append(name);
    mangled_name.append(descriptor);
    return mangled_name;
  }
};

/**
 * Produces names that are valid C identifiers. Uses JNI's name mangling for classes and methods (JNI only defines
 * mangling for those).
 * https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/design.html
 */
class JNINameMangler : public NameMangler {
 public:
  [[nodiscard]] std::string MangleFullyQualifiedClassName(const std::string_view class_name) const override {
    std::string mangled_name(class_name);
    std::replace(mangled_name.begin(), mangled_name.end(), '.', '/');
    mangled_name = MangleUnicodeCharacters(mangled_name);
    return mangled_name;
  }
  [[nodiscard]] std::string MangleMethodName(const std::string_view class_name, const std::string_view name,
                                             const std::string_view descriptor) const override {
    std::string mangled_name("Java_");
    mangled_name += this->MangleFullyQualifiedClassName(class_name);
    mangled_name += '_';
    mangled_name += MangleUnicodeCharacters(name);
    mangled_name += "__";
    mangled_name += MangleMethodDescriptor(descriptor);
    return mangled_name;
  }
  [[nodiscard]] std::string MangleVirtualDispatchName(std::string_view class_name, std::string_view name,
                                                      std::string_view descriptor) const override {
    // Not defined in JNI
    // v = virtual
    std::string mangled_name("Magnetic_v_");
    mangled_name += this->MangleFullyQualifiedClassName(class_name);
    mangled_name += '_';
    mangled_name += MangleUnicodeCharacters(name);
    mangled_name += "__";
    mangled_name += MangleMethodDescriptor(descriptor);
    return mangled_name;
  }
  [[nodiscard]] std::string MangleVTableName(std::string_view subclass, std::string_view base_class) const override {
    // Not defined in JNI
    // vt = vtable
    std::string mangled_name("Magnetic_vt_");
    mangled_name += this->MangleFullyQualifiedClassName(subclass);
    mangled_name += "__";
    mangled_name += this->MangleFullyQualifiedClassName(base_class);
    return mangled_name;
  }
  [[nodiscard]] std::string MangleStaticFieldName(const std::string_view class_name, const std::string_view name,
                                                  const std::string_view descriptor) const override {
    // Not defined in JNI
    // sf = static field
    std::string mangled_name("Magnetic_sf_");
    mangled_name += this->MangleFullyQualifiedClassName(class_name);
    mangled_name += '_';
    mangled_name += MangleUnicodeCharacters(name);
    mangled_name += "__";
    mangled_name += MangleUnicodeCharacters(descriptor);
    return mangled_name;
  }
  [[nodiscard]] std::string MangleInstanceFieldGetter(const std::string_view class_name, const std::string_view name,
                                                      const std::string_view descriptor) const override {
    // Not defined in JNI
    // ifg = instance field getter
    std::string mangled_name("Magnetic_ifg_");
    mangled_name += this->MangleFullyQualifiedClassName(class_name);
    mangled_name += '_';
    mangled_name += MangleUnicodeCharacters(name);
    mangled_name += "__";
    mangled_name += MangleUnicodeCharacters(descriptor);
    return mangled_name;
  }
  [[nodiscard]] std::string MangleInstanceFieldSetter(const std::string_view class_name, const std::string_view name,
                                                      const std::string_view descriptor) const override {
    // Not defined in JNI
    // ifs = instance field setter
    std::string mangled_name("Magnetic_ifs_");
    mangled_name += this->MangleFullyQualifiedClassName(class_name);
    mangled_name += '_';
    mangled_name += MangleUnicodeCharacters(name);
    mangled_name += "__";
    mangled_name += MangleUnicodeCharacters(descriptor);
    return mangled_name;
  }
  [[nodiscard]] std::string MangleInstantiatorName(const std::string_view class_name) const override {
    // Not defined in JNI
    return "Magnetic_new_" + this->MangleFullyQualifiedClassName(class_name);
  }

 private:
  static bool IsValidCIdentifier(char c) { return std::isalnum(c) || (c == '_'); }
  /**
   * https://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/design.html#wp133
   */
  static std::string MangleUnicodeCharacters(const std::string_view name) {
    // TODO: Unicode UTF-8
    std::string s;
    for (char c : name) {
      if (IsValidCIdentifier(c)) {
        s += c;
      } else if (c == '/') {
        s += '_';
      } else if (c == '_') {
        s += "_1";
      } else if (c == ';') {
        s += "_2";
      } else if (c == '[') {
        s += "_3";
      } else {
        s += "_0";
        s += fmt::format("{:04x}", static_cast<uint16_t>(c));
      }
    }
    return s;
  }
  static std::string MangleMethodDescriptor(const std::string_view descriptor) {
    // Method descriptors are mangled by taking the parameters then mangling unicode characters.
    // E.g. (ILjava/lang/String;)V -> ILjava/lang/String; -> ILjava_lang_String_2
    size_t params_start_index = 1;
    size_t params_end_index = descriptor.rfind(')');
    std::string_view params = descriptor.substr(params_start_index, params_end_index - params_start_index);
    return MangleUnicodeCharacters(params);
  }
};

}// namespace

std::unique_ptr<NameMangler> NameMangler::CreateSimpleMangler() { return std::make_unique<SimpleNameMangler>(); }
std::unique_ptr<NameMangler> NameMangler::CreateJNIMangler() { return std::make_unique<JNINameMangler>(); }

}// namespace magnetic
