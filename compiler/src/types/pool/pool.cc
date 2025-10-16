//
// Created by lunbun on 6/9/2022.
//

#include "pool.h"

#include <memory>

#include <cjbp/cjbp.h>

#include "class/class.h"
#include "context/context.h"

namespace magnetic {

ClassPool::~ClassPool() noexcept = default;

ClassInfo *ClassPool::Get(const std::string &class_name) {
  const auto &it = this->classes_.find(class_name);
  if (it != this->classes_.end()) return it->second.get();

  std::unique_ptr<cjbp::DataInputStream> stream = this->path_->Find(class_name);
  if (stream == nullptr) return nullptr;
  auto class_bytecode = std::make_unique<cjbp::Class>(*stream);

  auto unique_class = std::make_unique<ClassInfo>(this->ctx_, std::move(class_bytecode),
                                                  this->ctx_->CreateCompilationUnitForClass(class_name));
  this->classes_.emplace(class_name, std::move(unique_class));
  ClassInfo *clazz = this->classes_.at(class_name).get();
  clazz->EmitDefinition();
  return clazz;
}

}// namespace magnetic
