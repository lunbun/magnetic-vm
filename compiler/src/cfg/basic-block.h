//
// Created by lunbun on 6/11/2022.
//

#pragma once

#include <llvm/IR/BasicBlock.h>
#include <cjbp/cjbp.h>

namespace magnetic {

class BasicBlock {
 public:
  BasicBlock(int32_t start, int32_t end)
      : start_(start), end_(end), llvm_block_(nullptr) {}

  [[nodiscard]] int32_t start() const { return this->start_; }
  [[nodiscard]] int32_t end() const { return this->end_; }
  [[nodiscard]] llvm::BasicBlock *llvm_block() const;
  void set_llvm_block(llvm::BasicBlock *block) { this->llvm_block_ = block; }

 private:
  int32_t start_, end_;
  llvm::BasicBlock *llvm_block_;
};

}
