//
// Created by lunbun on 6/11/2022.
//

#include "basic-block.h"

#include <cassert>

namespace magnetic {

llvm::BasicBlock *BasicBlock::llvm_block() const {
  assert(this->llvm_block_ != nullptr);
  return this->llvm_block_;
}

}
