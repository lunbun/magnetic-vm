//
// Created by lunbun on 6/10/2022.
//

#include "codegen-method.h"

#include <fmt/core.h>
#include <llvm/IR/BasicBlock.h>

#include "class/descriptor.h"
#include "environment.h"
#include "instructions.h"
#include "types/type.h"

namespace magnetic {

namespace {
void EmitBasicBlocks(codegen::Environment &env) {
  for (auto &it : env.cfg().blocks()) {
    std::string block_name = "block" + std::to_string(it.first);
    llvm::BasicBlock *block = llvm::BasicBlock::Create(*env.ctx()->llvm_ctx(), block_name, env.function());
    it.second.set_llvm_block(block);
  }
}

void EmitCopyParameter(codegen::Environment &env, int32_t &java_index, int32_t &llvm_index, Type param_type) {
  codegen::TypedLocal &local = env.locals().Get(java_index, param_type);
  llvm::Value *param = env.function()->getArg(llvm_index);
  local.EmitStore(env.builder(), {param, param_type});

  java_index += param_type.width();
  ++llvm_index;
}
void EmitCopyAllParameters(codegen::Environment &env) {
  llvm::BasicBlock *param_block =
      llvm::BasicBlock::Create(*env.ctx()->llvm_ctx(), "params", env.function(), env.cfg().entry_block().llvm_block());
  env.builder().SetInsertPoint(env.locals().alloca_block());
  env.builder().CreateBr(param_block);

  env.builder().SetInsertPoint(param_block);
  MethodDescriptor *descriptor = env.method()->descriptor();
  int32_t java_index = 0;
  int32_t llvm_index = 0;
  for (Type param_type : descriptor->instance_params()) { EmitCopyParameter(env, java_index, llvm_index, param_type); }
  env.builder().CreateBr(env.cfg().entry_block().llvm_block());
}
}// namespace

void codegen::EmitMethod(ClassInfo *owner, MethodDeclaration *method, cjbp::Method *bytecode, llvm::Function *function,
                         llvm::Module *module) {
  Environment env(owner, method, bytecode, function, module);
  EmitBasicBlocks(env);
  EmitCopyAllParameters(env);
  for (const auto &it : env.cfg().blocks()) {
    const BasicBlock &block = it.second;
    env.iterator().MoveTo(block.start());
    env.builder().SetInsertPoint(block.llvm_block());
    while (env.iterator().position() < block.end()) { EmitInstruction(env); }

    // It is possible for a basic block to not end with a jump instruction if it was split due to there being
    // an instruction that jumps there.
    //
    // LLVM requires that all basic blocks end with a terminator (e.g. jump to another block, return from the
    // function, etc). This is kind of a hack, but we can just check if the basic block has a terminator and if
    // it doesn't, jump to the next basic block.
    if (block.llvm_block()->getTerminator() == nullptr) {
      env.builder().CreateBr(env.cfg().GetBlock(block.end()).llvm_block());
    }
  }
}

}// namespace magnetic
