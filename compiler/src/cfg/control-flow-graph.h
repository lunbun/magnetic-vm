//
// Created by lunbun on 6/11/2022.
//

#pragma once

#include <cstdint>
#include <map>
#include <memory>

#include <cjbp/cjbp.h>

#include "basic-block.h"

namespace magnetic {

class ControlFlowGraph {
 public:
  explicit ControlFlowGraph(cjbp::CodeIterator &it);

  [[nodiscard]] BasicBlock &entry_block() const;
  [[nodiscard]] auto &blocks() { return this->blocks_; }
  [[nodiscard]] BasicBlock &GetBlock(int32_t inst);

 private:
  BasicBlock *entry_block_;
  std::map<int32_t, BasicBlock> blocks_;
};

}// namespace magnetic
