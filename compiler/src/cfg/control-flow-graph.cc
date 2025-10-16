//
// Created by lunbun on 6/11/2022.
//

#include "control-flow-graph.h"

#include <algorithm>
#include <memory>
#include <unordered_set>

#include "basic-block.h"

namespace magnetic {

ControlFlowGraph::ControlFlowGraph(cjbp::CodeIterator &it) : entry_block_(nullptr), blocks_() {
  std::unordered_set<int32_t> leaders{};
  leaders.insert(0);
  while (it.HasNext()) {
    size_t index = it.Next();
    uint8_t opcode = it.ReadUInt8(index);
    if (cjbp::Opcode::kIfEq <= opcode && opcode <= cjbp::Opcode::kGoto) {
      leaders.insert(static_cast<int32_t>(index + it.ReadInt16(index + 1)));
      leaders.insert(static_cast<int32_t>(it.LookAhead()));
    }
  }

  std::vector<int32_t> sorted_leaders(leaders.begin(), leaders.end());
  std::sort(sorted_leaders.begin(), sorted_leaders.end());
  for (size_t i = 0; i < sorted_leaders.size(); ++i) {
    int32_t start = sorted_leaders[i];
    int32_t end = (i + 1 >= sorted_leaders.size() ? static_cast<int32_t>(it.code_length()) : sorted_leaders[i + 1]);

    this->blocks_.emplace(start, BasicBlock(start, end));
  }
  this->entry_block_ = &this->blocks_.at(0);
}

BasicBlock &ControlFlowGraph::entry_block() const {
  assert(this->entry_block_ != nullptr);
  return *this->entry_block_;
}
BasicBlock &ControlFlowGraph::GetBlock(int32_t inst) {
  return this->blocks().at(inst);
}

}
