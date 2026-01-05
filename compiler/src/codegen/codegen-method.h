//
// Created by lunbun on 6/10/2022.
//

#pragma once

#include "types/class/class.h"
#include "types/class/method.h"

namespace magnetic::codegen {

void EmitMethod(ClassInfo *owner, MethodDeclaration *method, cjbp::Method *bytecode, llvm::Function *function,
                llvm::Module *module);

}
