//
// Created by lunbun on 7/12/2022.
//

#include "runtime-abi.h"

#include <functional>
#include <map>

#include "class/mangle.h"
#include "context/context.h"

namespace magnetic {

RuntimeABI::RuntimeABI() : ctx_(nullptr) {}

namespace {
class DefaultRuntimeABI : public RuntimeABI {
 public:
  ~DefaultRuntimeABI() noexcept override = default;

  Value GetStringConstant(llvm::IRBuilder<> &builder, const std::string_view value) override {
    llvm::Module *module = builder.GetInsertBlock()->getModule();
    llvm::Function *getter = this->GetStringLiteralGetterInModule(module, value);

    llvm::Value *string_literal = builder.CreateCall(getter, llvm::None, "string_literal");
    return {string_literal, Type::kObject};
  }

 private:
  std::map<llvm::Module *, std::map<std::string, llvm::Function *, std::less<>>> string_literal_getters_;

  [[nodiscard]] llvm::FunctionType *getter_type() const {
    return llvm::FunctionType::get(this->ctx()->ptr_type(), llvm::None, false);
  }

  [[nodiscard]] llvm::FunctionCallee GetStringPoolLookupFunctionInModule(llvm::Module *module) const {
    static constexpr const char *kStringPoolLookupName = "Magnetic_rt_string_pool_get";

    std::vector<llvm::Type *> arg_types;
    arg_types.reserve(2);
    arg_types.push_back(this->ctx()->int32());   // length
    arg_types.push_back(this->ctx()->ptr_type());// char pointer
    llvm::FunctionType *function_type = llvm::FunctionType::get(this->ctx()->ptr_type(), arg_types, false);

    // TODO: add attributes to function
    llvm::FunctionCallee function = module->getOrInsertFunction(kStringPoolLookupName, function_type);
    return function;
  }
  [[nodiscard]] llvm::Value *EmitStringPoolLookup(llvm::IRBuilder<> &builder, const std::string_view value,
                                                  const std::string_view name) const {
    llvm::Module *module = builder.GetInsertBlock()->getModule();

    llvm::Constant *char_array = llvm::ConstantDataArray::getString(*this->ctx()->llvm_ctx(), value, false);
    auto *char_ptr = new llvm::GlobalVariable(*module, char_array->getType(), true, llvm::GlobalValue::PrivateLinkage,
                                              char_array, ".str");

    llvm::FunctionCallee function = this->GetStringPoolLookupFunctionInModule(module);
    std::vector<llvm::Value *> args;
    args.push_back(llvm::ConstantInt::get(this->ctx()->int32(), value.size()));
    args.push_back(char_ptr);
    llvm::Value *string_ptr = builder.CreateCall(function, args, name);
    return string_ptr;
  }

  llvm::Function *GetStringLiteralGetterInModule(llvm::Module *module, const std::string_view value) {
    auto &module_getters = this->string_literal_getters_[module];
    const auto &it = module_getters.find(value);
    if (it != module_getters.end()) return it->second;

    llvm::Function *function = this->CreateStringLiteralGetterInModule(module, value);
    module_getters.emplace(std::string(value), function);
    return function;
  }
  [[nodiscard]] llvm::Function *CreateStringLiteralGetterInModule(llvm::Module *module,
                                                                  const std::string_view value) const {
    // Since strings in Java are immutable, all string literals with the same value have to have the same pointer. This
    // is accomplished with a string pool.
    //
    // We generate a function that gets the string literal for the string at the constant pool index. This function first
    // checks a global variable used to cache the string literal. This global variable allows us to avoid having to do a
    // lookup in the string pool each time we want to get this string literal. If the global variable doesn't contain the
    // string, we do a lookup in the string pool (and probably add a new string to it), then cache the string.
    llvm::Function *function =
        llvm::Function::Create(this->getter_type(), llvm::GlobalValue::ExternalLinkage, ".str.get", module);
    function->addRetAttr(llvm::Attribute::NoUndef);
    function->addRetAttr(llvm::Attribute::NonNull);

    auto *string_cache =
        new llvm::GlobalVariable(*module, this->ctx()->ptr_type(), false, llvm::GlobalValue::PrivateLinkage,
                                 this->ctx()->pointer_null(), ".str.cache");

    llvm::IRBuilder<> builder(*this->ctx()->llvm_ctx());
    llvm::BasicBlock *entry = llvm::BasicBlock::Create(*this->ctx()->llvm_ctx(), "entry", function);
    llvm::BasicBlock *pool_lookup = llvm::BasicBlock::Create(*this->ctx()->llvm_ctx(), "pool_lookup", function);
    llvm::BasicBlock *exit = llvm::BasicBlock::Create(*this->ctx()->llvm_ctx(), "exit", function);

    builder.SetInsertPoint(entry);
    llvm::Value *cached_string = builder.CreateLoad(this->ctx()->ptr_type(), string_cache, "cached_string");
    llvm::Value *needs_lookup = builder.CreateICmpEQ(cached_string, this->ctx()->pointer_null(), "needs_lookup");
    builder.CreateCondBr(needs_lookup, pool_lookup, exit);

    builder.SetInsertPoint(pool_lookup);
    llvm::Value *string_ptr = EmitStringPoolLookup(builder, value, "string_ptr");
    builder.CreateBr(exit);

    builder.SetInsertPoint(exit);
    llvm::PHINode *result = builder.CreatePHI(this->ctx()->ptr_type(), 2, "result");
    result->addIncoming(string_ptr, pool_lookup);
    result->addIncoming(cached_string, entry);
    builder.CreateRet(result);
    return function;
  }
};
}// namespace

std::unique_ptr<RuntimeABI> RuntimeABI::CreateDefaultABI() { return std::make_unique<DefaultRuntimeABI>(); }

}// namespace magnetic
