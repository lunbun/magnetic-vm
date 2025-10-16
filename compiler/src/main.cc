#include <memory>

#include "class/class.h"
#include "class/mangle.h"
#include "class/pool/path.h"
#include "class/pool/pool.h"
#include "codegen/runtime-abi.h"
#include "compilation-unit/compilation-unit.h"
#include "context/context.h"

int main() {
  magnetic::Context ctx{};
  std::vector<std::unique_ptr<magnetic::ClassPath>> class_paths{};
  class_paths.push_back(magnetic::ClassPath::CreateJarClassPath("resources/test.jar"));
  //  class_paths.push_back(magnetic::ClassPath::CreateJarClassPath("resources/rt.jar"));
  ctx.set_pool(
      std::make_unique<magnetic::ClassPool>(magnetic::ClassPath::CreateCompositeClassPath(std::move(class_paths))));

  ctx.set_name_mangler(magnetic::NameMangler::CreateJNIMangler());
  ctx.set_runtime_abi(magnetic::RuntimeABI::CreateDefaultABI());
  ctx.set_use_single_unit(true);
  ctx.pool()->Get("io.github.lunbun.Main");

  ctx.global_unit()->Verify();
  ctx.global_unit()->Optimize(llvm::OptimizationLevel::O2);
  ctx.global_unit()->PrintModuleToFile("resources/Test.ll");
}
