//
// Created by lunbun on 6/12/2022.
//

#include "instructions.h"

#include <algorithm>
#include <vector>

#include <cjbp/cjbp.h>
#include <fmt/core.h>
#include <llvm/IR/Instructions.h>

#include "class/class.h"
#include "class/descriptor.h"
#include "class/field.h"
#include "class/instantiate.h"
#include "class/method.h"
#include "compilation-unit/compilation-unit.h"
#include "context/exception.h"
#include "runtime-abi.h"
#include "types/type.h"

using namespace cjbp::Opcode;

namespace magnetic {

namespace {
void EmitIConst(codegen::Environment &env, int32_t num) {
  llvm::Value *value = llvm::ConstantInt::get(env.ctx()->int32(), num);
  env.stack().Push({value, Type::kInt});
}
void EmitLConst(codegen::Environment &env, int64_t num) {
  llvm::Value *value = llvm::ConstantInt::get(env.ctx()->int64(), num);
  env.stack().Push({value, Type::kLong});
}
void EmitFConst(codegen::Environment &env, float num) {
  llvm::Value *value = llvm::ConstantFP::get(env.ctx()->float32(), num);
  env.stack().Push({value, Type::kFloat});
}
void EmitDConst(codegen::Environment &env, double num) {
  llvm::Value *value = llvm::ConstantFP::get(env.ctx()->float64(), num);
  env.stack().Push({value, Type::kDouble});
}
void EmitStringConst(codegen::Environment &env, const std::string_view value) {
  Value string_ptr = env.ctx()->runtime_abi()->GetStringConstant(env.builder(), value);
  env.stack().Push(string_ptr);
}
void EmitLdc(codegen::Environment &env, uint16_t pool_index) {
  const cjbp::ConstPool &pool = env.clazz()->bytecode()->const_pool();
  std::optional<cjbp::ConstTag> tag = pool.GetTag(pool_index);
  if (tag == cjbp::ConstTag::kInteger) {
    EmitIConst(env, pool.GetInteger(pool_index).value());
  } else if (tag == cjbp::ConstTag::kFloat) {
    EmitFConst(env, pool.GetFloat(pool_index).value());
  } else if (tag == cjbp::ConstTag::kLong) {
    EmitLConst(env, pool.GetLong(pool_index).value());
  } else if (tag == cjbp::ConstTag::kDouble) {
    EmitDConst(env, pool.GetDouble(pool_index).value());
  } else if (tag == cjbp::ConstTag::kString) {
    EmitStringConst(env, *pool.GetString(pool_index));
  } else {
    if (!tag.has_value()) {
      throw BadBytecode("ldc references invalid const pool entry");
    } else {
      throw BadBytecode(fmt::format("unknown ldc tag {}", static_cast<int32_t>(tag.value())));
    }
  }
}

void EmitLocalLoad(codegen::Environment &env, const codegen::TypedLocal &local, const std::string &name) {
  env.stack().Push(local.EmitLoad(env.ctx(), env.builder(), name));
}
void EmitLocalStore(codegen::Environment &env, const codegen::TypedLocal &local) {
  // TODO: cast value to local type
  Value value = env.stack().Pop();
  local.EmitStore(env.builder(), value);
}

void EmitDup(codegen::Environment &env) {
  const Value &top = env.stack().Top();
  if (top.type.width() != 1) throw BadBytecode("can only dup values with width of 1");
  env.stack().Push(top);
}

void EmitBinaryOp(codegen::Environment &env, llvm::Instruction::BinaryOps op, Type type, const std::string &name) {
  Value rhs = env.stack().Pop();
  Value lhs = env.stack().Pop();
  env.stack().Push({env.builder().CreateBinOp(op, lhs.value, rhs.value, name), type});
}

void EmitScalarCast(codegen::Environment &env, llvm::Instruction::CastOps op, Type src, Type dest,
                    const std::string &name) {
  Value value = env.stack().Pop();
  env.stack().Push({env.builder().CreateCast(op, value.value, dest.llvm_type(env.ctx()), name), dest});
}

void EmitICmpBranch(codegen::Environment &env, llvm::CmpInst::Predicate op, size_t inst_offset, llvm::Value *lhs,
                    llvm::Value *rhs, const std::string &name) {
  llvm::Value *condition = env.builder().CreateICmp(op, lhs, rhs, name);
  BasicBlock &true_block =
      env.cfg().GetBlock(static_cast<int32_t>(inst_offset) + env.iterator().ReadInt16(inst_offset + 1));
  BasicBlock &false_block = env.cfg().GetBlock(static_cast<int32_t>(env.iterator().LookAhead()));
  env.builder().CreateCondBr(condition, true_block.llvm_block(), false_block.llvm_block());
}
void EmitComparisonBranch(codegen::Environment &env, llvm::CmpInst::Predicate op, size_t inst_offset,
                          const std::string &name) {
  Value rhs = env.stack().Pop();
  Value lhs = env.stack().Pop();
  EmitICmpBranch(env, op, inst_offset, lhs.value, rhs.value, name);
}
void EmitZeroComparisonBranch(codegen::Environment &env, llvm::CmpInst::Predicate op, size_t inst_offset,
                              const std::string &name) {
  Value lhs = env.stack().Pop();
  llvm::Value *rhs = llvm::ConstantInt::get(env.ctx()->int32(), 0, true);
  EmitICmpBranch(env, op, inst_offset, lhs.value, rhs, name);
}
void EmitGoto(codegen::Environment &env, size_t destination) {
  BasicBlock &block = env.cfg().GetBlock(static_cast<int32_t>(destination));
  env.builder().CreateBr(block.llvm_block());
}

void EmitReturn(codegen::Environment &env, Type type) {
  Value value = env.stack().Pop();
  env.builder().CreateRet(value.value);
}
void EmitObjectReturn(codegen::Environment &env) {
  Type return_type = env.method()->descriptor()->return_type();
  env.builder().CreateRet(env.stack().Pop().value);
}
void EmitVoidReturn(codegen::Environment &env) { env.builder().CreateRetVoid(); }

void EmitGetStatic(codegen::Environment &env, uint16_t pool_index) {
  const cjbp::ConstPool &pool = env.clazz()->bytecode()->const_pool();
  FieldDeclaration *target_field = env.ctx()->GetField(
      *pool.GetFieldRefClass(pool_index), *pool.GetFieldRefName(pool_index), *pool.GetFieldRefType(pool_index), true);
  Value field_value = target_field->EmitLoad(env.builder(), std::nullopt, "getstatic");
  env.stack().Push(field_value);
}

void EmitPutStatic(codegen::Environment &env, uint16_t pool_index) {
  const cjbp::ConstPool &pool = env.clazz()->bytecode()->const_pool();
  FieldDeclaration *target_field = env.ctx()->GetField(
      *pool.GetFieldRefClass(pool_index), *pool.GetFieldRefName(pool_index), *pool.GetFieldRefType(pool_index), true);
  Value field_value = env.stack().Pop();
  target_field->EmitStore(env.builder(), std::nullopt, field_value);
}

void EmitGetField(codegen::Environment &env, uint16_t pool_index) {
  const cjbp::ConstPool &pool = env.clazz()->bytecode()->const_pool();
  FieldDeclaration *target_field = env.ctx()->GetField(
      *pool.GetFieldRefClass(pool_index), *pool.GetFieldRefName(pool_index), *pool.GetFieldRefType(pool_index), true);

  Value object_ref = env.stack().Pop();
  // TODO: cast object_ref to java.lang.Object
  // TODO: null check
  Value field_value = target_field->EmitLoad(env.builder(), object_ref, "getfield");
  env.stack().Push(field_value);
}

void EmitPutField(codegen::Environment &env, uint16_t pool_index) {
  const cjbp::ConstPool &pool = env.clazz()->bytecode()->const_pool();
  FieldDeclaration *target_field = env.ctx()->GetField(
      *pool.GetFieldRefClass(pool_index), *pool.GetFieldRefName(pool_index), *pool.GetFieldRefType(pool_index), true);

  Value field_value = env.stack().Pop();
  Value object_ref = env.stack().Pop();
  // TODO: null check
  target_field->EmitStore(env.builder(), object_ref, field_value);
}

std::vector<Value> PopAllMethodParams(codegen::Environment &env, MethodDescriptor *descriptor) {
  std::vector<Value> params{};
  const std::vector<Type> &param_types = descriptor->descriptor_params();
  params.reserve(param_types.size());
  for (Type param_type : param_types) {
    // TODO: dont cast here
    params.push_back(env.stack().Pop());
  }
  std::reverse(params.begin(), params.end());
  return params;
}
void MaybePushCallResultOntoStack(codegen::Environment &env, Value call_result, const std::string &name) {
  if (call_result.type != Type::kVoid) {
    call_result.value->setName(name);
    env.stack().Push(call_result);
  }
}
void CreateCallAndMaybePushResultOntoStack(codegen::Environment &env, MethodDeclaration *target_method,
                                           std::optional<Value> object_ref, const std::vector<Value> &params,
                                           const std::string &name) {
  Value call_result = target_method->EmitCall(env.builder(), object_ref, params, "");
  MaybePushCallResultOntoStack(env, call_result, name);
}
void CreateNonVirtualInstanceInvoke(codegen::Environment &env, MethodDeclaration *target_method,
                                    const std::string &name) {
  std::vector<Value> params = PopAllMethodParams(env, target_method->descriptor());
  // TODO: cast to java.lang.Object
  Value object_ref = env.stack().Pop();
  // TODO: null check
  CreateCallAndMaybePushResultOntoStack(env, target_method, object_ref, params, name);
}
void CreateVirtualInstanceInvoke(codegen::Environment &env, MethodDeclaration *target_method, const std::string &name) {
  std::vector<Value> params = PopAllMethodParams(env, target_method->descriptor());
  // TODO: cast to java.lang.Object
  Value object_ref = env.stack().Pop();
  // TODO: null check
  Value call_result = target_method->EmitVirtualCall(env.builder(), object_ref, params, "");
  MaybePushCallResultOntoStack(env, call_result, name);
}

void EmitInvokeVirtualInst(codegen::Environment &env, uint16_t pool_index) {
  const cjbp::ConstPool &pool = env.clazz()->bytecode()->const_pool();
  MethodDeclaration *target_method =
      env.ctx()->GetMethod(*pool.GetMethodRefClass(pool_index), *pool.GetMethodRefName(pool_index),
                           *pool.GetMethodRefType(pool_index), false);
  if (!target_method->CanBeOverridden()) {
    CreateNonVirtualInstanceInvoke(env, target_method, "invokevirtual");
  } else {
    CreateVirtualInstanceInvoke(env, target_method, "invokevirtual");
  }
}

void EmitInvokeSpecialInst(codegen::Environment &env, uint16_t pool_index) {
  const cjbp::ConstPool &pool = env.clazz()->bytecode()->const_pool();
  MethodDeclaration *target_method =
      env.ctx()->GetMethod(*pool.GetMethodRefClass(pool_index), *pool.GetMethodRefName(pool_index),
                           *pool.GetMethodRefType(pool_index), false);
  CreateNonVirtualInstanceInvoke(env, target_method, "invokespecial");
}

void EmitInvokeStaticInst(codegen::Environment &env, uint16_t pool_index) {
  const cjbp::ConstPool &pool = env.clazz()->bytecode()->const_pool();
  MethodDeclaration *target_method =
      env.ctx()->GetMethod(*pool.GetMethodRefClass(pool_index), *pool.GetMethodRefName(pool_index),
                           *pool.GetMethodRefType(pool_index), true);

  std::vector<Value> params = PopAllMethodParams(env, target_method->descriptor());
  CreateCallAndMaybePushResultOntoStack(env, target_method, std::nullopt, params, "invokestatic");
}

void EmitNew(codegen::Environment &env, uint16_t pool_index) {
  const cjbp::ConstPool &pool = env.clazz()->bytecode()->const_pool();
  ClassInstantiator *instantiator = env.ctx()->GetInstantiator(*pool.GetClassName(pool_index));
  Value instance = instantiator->EmitInstantiation(env.builder(), "new");
  env.stack().Push(instance);
}
}// namespace

void codegen::EmitInstruction(codegen::Environment &env) {
  size_t index = env.iterator().Next();
  uint8_t opcode = env.iterator().ReadUInt8(index);
  switch (opcode) {
    case Opcode::kNop: break;

    case Opcode::kIConstM1: EmitIConst(env, -1); break;
    case Opcode::kIConst0: EmitIConst(env, 0); break;
    case Opcode::kIConst1: EmitIConst(env, 1); break;
    case Opcode::kIConst2: EmitIConst(env, 2); break;
    case Opcode::kIConst3: EmitIConst(env, 3); break;
    case Opcode::kIConst4: EmitIConst(env, 4); break;
    case Opcode::kIConst5: EmitIConst(env, 5); break;
    case Opcode::kLConst0: EmitLConst(env, 0); break;
    case Opcode::kLConst1: EmitLConst(env, 1); break;
    case Opcode::kFConst0: EmitFConst(env, 0); break;
    case Opcode::kFConst1: EmitFConst(env, 1); break;
    case Opcode::kFConst2: EmitFConst(env, 2); break;
    case Opcode::kDConst0: EmitDConst(env, 0); break;
    case Opcode::kDConst1: EmitDConst(env, 1); break;
    case Opcode::kBIPush: EmitIConst(env, env.iterator().ReadInt8(index + 1)); break;
    case Opcode::kSIPush: EmitIConst(env, env.iterator().ReadInt16(index + 1)); break;
    case Opcode::kLdc: EmitLdc(env, env.iterator().ReadUInt8(index + 1)); break;
    case Opcode::kLdcW:
    case Opcode::kLdc2W: EmitLdc(env, env.iterator().ReadUInt16(index + 1)); break;

    case Opcode::kILoad0: EmitLocalLoad(env, env.locals().GetInt(0), "iload_0"); break;
    case Opcode::kILoad1: EmitLocalLoad(env, env.locals().GetInt(1), "iload_1"); break;
    case Opcode::kILoad2: EmitLocalLoad(env, env.locals().GetInt(2), "iload_2"); break;
    case Opcode::kILoad3: EmitLocalLoad(env, env.locals().GetInt(3), "iload_3"); break;
    case Opcode::kLLoad0: EmitLocalLoad(env, env.locals().GetLong(0), "lload_0"); break;
    case Opcode::kLLoad1: EmitLocalLoad(env, env.locals().GetLong(1), "lload_1"); break;
    case Opcode::kLLoad2: EmitLocalLoad(env, env.locals().GetLong(2), "lload_2"); break;
    case Opcode::kLLoad3: EmitLocalLoad(env, env.locals().GetLong(3), "lload_3"); break;
    case Opcode::kFLoad0: EmitLocalLoad(env, env.locals().GetFloat(0), "fload_0"); break;
    case Opcode::kFLoad1: EmitLocalLoad(env, env.locals().GetFloat(1), "fload_1"); break;
    case Opcode::kFLoad2: EmitLocalLoad(env, env.locals().GetFloat(2), "fload_2"); break;
    case Opcode::kFLoad3: EmitLocalLoad(env, env.locals().GetFloat(3), "fload_3"); break;
    case Opcode::kDLoad0: EmitLocalLoad(env, env.locals().GetDouble(0), "dload_0"); break;
    case Opcode::kDLoad1: EmitLocalLoad(env, env.locals().GetDouble(1), "dload_1"); break;
    case Opcode::kDLoad2: EmitLocalLoad(env, env.locals().GetDouble(2), "dload_2"); break;
    case Opcode::kDLoad3: EmitLocalLoad(env, env.locals().GetDouble(3), "dload_3"); break;
    case Opcode::kALoad0: EmitLocalLoad(env, env.locals().GetObject(0), "aload_0"); break;
    case Opcode::kALoad1: EmitLocalLoad(env, env.locals().GetObject(1), "aload_1"); break;
    case Opcode::kALoad2: EmitLocalLoad(env, env.locals().GetObject(2), "aload_2"); break;
    case Opcode::kALoad3: EmitLocalLoad(env, env.locals().GetObject(3), "aload_3"); break;

    case Opcode::kIStore0: EmitLocalStore(env, env.locals().GetInt(0)); break;
    case Opcode::kIStore1: EmitLocalStore(env, env.locals().GetInt(1)); break;
    case Opcode::kIStore2: EmitLocalStore(env, env.locals().GetInt(2)); break;
    case Opcode::kIStore3: EmitLocalStore(env, env.locals().GetInt(3)); break;
    case Opcode::kLStore0: EmitLocalStore(env, env.locals().GetLong(0)); break;
    case Opcode::kLStore1: EmitLocalStore(env, env.locals().GetLong(1)); break;
    case Opcode::kLStore2: EmitLocalStore(env, env.locals().GetLong(2)); break;
    case Opcode::kLStore3: EmitLocalStore(env, env.locals().GetLong(3)); break;
    case Opcode::kFStore0: EmitLocalStore(env, env.locals().GetFloat(0)); break;
    case Opcode::kFStore1: EmitLocalStore(env, env.locals().GetFloat(1)); break;
    case Opcode::kFStore2: EmitLocalStore(env, env.locals().GetFloat(2)); break;
    case Opcode::kFStore3: EmitLocalStore(env, env.locals().GetFloat(3)); break;
    case Opcode::kDStore0: EmitLocalStore(env, env.locals().GetDouble(0)); break;
    case Opcode::kDStore1: EmitLocalStore(env, env.locals().GetDouble(1)); break;
    case Opcode::kDStore2: EmitLocalStore(env, env.locals().GetDouble(2)); break;
    case Opcode::kDStore3: EmitLocalStore(env, env.locals().GetDouble(3)); break;
    case Opcode::kAStore0: EmitLocalStore(env, env.locals().GetObject(0)); break;
    case Opcode::kAStore1: EmitLocalStore(env, env.locals().GetObject(1)); break;
    case Opcode::kAStore2: EmitLocalStore(env, env.locals().GetObject(2)); break;
    case Opcode::kAStore3: EmitLocalStore(env, env.locals().GetObject(3)); break;

    case Opcode::kDup: EmitDup(env); break;

    case Opcode::kIAdd: EmitBinaryOp(env, llvm::Instruction::BinaryOps::Add, Type::kInt, "iadd"); break;
    case Opcode::kIMul: EmitBinaryOp(env, llvm::Instruction::BinaryOps::Mul, Type::kInt, "imul"); break;

    case Opcode::kI2L: EmitScalarCast(env, llvm::Instruction::CastOps::SExt, Type::kInt, Type::kLong, "i2l"); break;
    case Opcode::kI2F: EmitScalarCast(env, llvm::Instruction::CastOps::SIToFP, Type::kInt, Type::kFloat, "i2f"); break;
    case Opcode::kI2D: EmitScalarCast(env, llvm::Instruction::CastOps::SIToFP, Type::kInt, Type::kDouble, "i2d"); break;
    case Opcode::kL2I: EmitScalarCast(env, llvm::Instruction::CastOps::Trunc, Type::kLong, Type::kInt, "l2i"); break;
    case Opcode::kL2F: EmitScalarCast(env, llvm::Instruction::CastOps::SIToFP, Type::kLong, Type::kFloat, "l2f"); break;
    case Opcode::kL2D:
      EmitScalarCast(env, llvm::Instruction::CastOps::SIToFP, Type::kLong, Type::kDouble, "l2d");
      break;
    case Opcode::kF2I: EmitScalarCast(env, llvm::Instruction::CastOps::FPToSI, Type::kFloat, Type::kInt, "f2i"); break;
    case Opcode::kF2L: EmitScalarCast(env, llvm::Instruction::CastOps::FPToSI, Type::kFloat, Type::kLong, "f2l"); break;
    case Opcode::kF2D:
      EmitScalarCast(env, llvm::Instruction::CastOps::FPExt, Type::kFloat, Type::kDouble, "f2d");
      break;
    case Opcode::kD2I: EmitScalarCast(env, llvm::Instruction::CastOps::FPToSI, Type::kDouble, Type::kInt, "d2i"); break;
    case Opcode::kD2L:
      EmitScalarCast(env, llvm::Instruction::CastOps::FPToSI, Type::kDouble, Type::kLong, "d2l");
      break;
    case Opcode::kD2F:
      EmitScalarCast(env, llvm::Instruction::CastOps::FPTrunc, Type::kDouble, Type::kFloat, "d2f");
      break;

    case Opcode::kIfEq: EmitZeroComparisonBranch(env, llvm::CmpInst::ICMP_EQ, index, "ifeq"); break;
    case Opcode::kIfNe: EmitZeroComparisonBranch(env, llvm::CmpInst::ICMP_NE, index, "ifne"); break;
    case Opcode::kIfLt: EmitZeroComparisonBranch(env, llvm::CmpInst::ICMP_SLT, index, "iflt"); break;
    case Opcode::kIfGe: EmitZeroComparisonBranch(env, llvm::CmpInst::ICMP_SGE, index, "ifge"); break;
    case Opcode::kIfGt: EmitZeroComparisonBranch(env, llvm::CmpInst::ICMP_SGT, index, "ifgt"); break;
    case Opcode::kIfLe: EmitZeroComparisonBranch(env, llvm::CmpInst::ICMP_SLE, index, "ifle"); break;
    case Opcode::kIfICmpEq: EmitComparisonBranch(env, llvm::CmpInst::ICMP_EQ, index, "if_icmpeq"); break;
    case Opcode::kIfICmpNe: EmitComparisonBranch(env, llvm::CmpInst::ICMP_NE, index, "if_icmpne"); break;
    case Opcode::kIfICmpLt: EmitComparisonBranch(env, llvm::CmpInst::ICMP_SLT, index, "if_icmplt"); break;
    case Opcode::kIfICmpGe: EmitComparisonBranch(env, llvm::CmpInst::ICMP_SGE, index, "if_icmpge"); break;
    case Opcode::kIfICmpGt: EmitComparisonBranch(env, llvm::CmpInst::ICMP_SGT, index, "if_icmpgt"); break;
    case Opcode::kIfICmpLe: EmitComparisonBranch(env, llvm::CmpInst::ICMP_SLE, index, "if_icmple"); break;
    case Opcode::kIfACmpEq: EmitComparisonBranch(env, llvm::CmpInst::ICMP_EQ, index, "if_acmpeq"); break;
    case Opcode::kIfACmpNe: EmitComparisonBranch(env, llvm::CmpInst::ICMP_NE, index, "if_acmpne"); break;
    case Opcode::kGoto: EmitGoto(env, index + env.iterator().ReadInt16(index + 1)); break;

    case Opcode::kIReturn: EmitReturn(env, Type::kInt); break;
    case Opcode::kLReturn: EmitReturn(env, Type::kLong); break;
    case Opcode::kFReturn: EmitReturn(env, Type::kFloat); break;
    case Opcode::kDReturn: EmitReturn(env, Type::kDouble); break;
    case Opcode::kAReturn: EmitObjectReturn(env); break;
    case Opcode::kReturn: EmitVoidReturn(env); break;

    case Opcode::kGetStatic: EmitGetStatic(env, env.iterator().ReadUInt16(index + 1)); break;
    case Opcode::kPutStatic: EmitPutStatic(env, env.iterator().ReadUInt16(index + 1)); break;
    case Opcode::kGetField: EmitGetField(env, env.iterator().ReadUInt16(index + 1)); break;
    case Opcode::kPutField: EmitPutField(env, env.iterator().ReadUInt16(index + 1)); break;
    case Opcode::kInvokeVirtual: EmitInvokeVirtualInst(env, env.iterator().ReadUInt16(index + 1)); break;
    case Opcode::kInvokeSpecial: EmitInvokeSpecialInst(env, env.iterator().ReadUInt16(index + 1)); break;
    case Opcode::kInvokeStatic: EmitInvokeStaticInst(env, env.iterator().ReadUInt16(index + 1)); break;
    case Opcode::kNew: EmitNew(env, env.iterator().ReadUInt16(index + 1)); break;

    default: throw BadBytecode(fmt::format("unknown opcode {:#04x}", opcode));
  }
}

}// namespace magnetic
