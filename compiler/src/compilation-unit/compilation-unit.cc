//
// Created by lunbun on 6/16/2022.
//

#include "compilation-unit.h"

#include <llvm/Analysis/CGSCCPassManager.h>
#include <llvm/Analysis/LoopAccessAnalysis.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>

#include "class/class.h"
#include "context/context.h"

namespace magnetic {

CompilationUnit::CompilationUnit(std::string module_name, Context *ctx)
    : ctx_(ctx), module_name_(std::move(module_name)) {
  this->module_ = new llvm::Module(this->module_name_, *this->ctx_->llvm_ctx());
}

void CompilationUnit::Verify() const { llvm::verifyModule(*this->module_, &llvm::errs()); }
void CompilationUnit::Optimize(llvm::OptimizationLevel level) const {
  llvm::LoopAnalysisManager loop_analysis;
  llvm::FunctionAnalysisManager function_analysis;
  llvm::CGSCCAnalysisManager call_graph_analysis;
  llvm::ModuleAnalysisManager module_analysis;
  llvm::PassBuilder pass_builder;
  pass_builder.registerModuleAnalyses(module_analysis);
  pass_builder.registerCGSCCAnalyses(call_graph_analysis);
  pass_builder.registerFunctionAnalyses(function_analysis);
  pass_builder.registerLoopAnalyses(loop_analysis);
  pass_builder.crossRegisterProxies(loop_analysis, function_analysis, call_graph_analysis, module_analysis);
  llvm::ModulePassManager pass_manager = pass_builder.buildPerModuleDefaultPipeline(level);
  pass_manager.run(*this->module_, module_analysis);
}
void CompilationUnit::PrintModuleToFile(const std::string &path) const {
  std::error_code ec;
  llvm::raw_fd_ostream ofs(path, ec);
  this->module_->print(ofs, nullptr);
  ofs.close();
}

}// namespace magnetic
