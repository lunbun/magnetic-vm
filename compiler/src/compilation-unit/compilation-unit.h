//
// Created by lunbun on 6/16/2022.
//

#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <llvm/IR/Module.h>
#include <llvm/Passes/OptimizationLevel.h>

namespace magnetic {

class Context;

class CompilationUnit {
 public:
  CompilationUnit(std::string module_name, Context *ctx);

  void Verify() const;
  void Optimize(llvm::OptimizationLevel level) const;
  void PrintModuleToFile(const std::string &path) const;

  [[nodiscard]] llvm::Module *module() const { return this->module_; }

 private:
  Context *ctx_;
  llvm::Module *module_;
  std::string module_name_;
};

}// namespace magnetic
