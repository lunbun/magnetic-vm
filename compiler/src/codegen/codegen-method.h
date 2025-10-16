//
// Created by lunbun on 6/10/2022.
//

#pragma once

#include "class/class.h"
#include "class/method.h"

namespace magnetic::codegen {

void EmitMethod(ClassInfo *owner, MethodDeclaration *method, cjbp::Method *bytecode, llvm::Function *function,
                llvm::Module *module);

}
