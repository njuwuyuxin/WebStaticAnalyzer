#include "checkers/CharArrayBound.h"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

void CharArrayBound::check() {
  getEntryFunc();
  if (entryFunc != nullptr) {
    FunctionDecl *funDecl = manager->getFunctionDecl(entryFunc);
    std::cout << "The entry function is: "
              << funDecl->getQualifiedNameAsString() << std::endl;
    std::cout << "Here is its dump: " << std::endl;
    funDecl->dump();
  }
  LangOptions LangOpts;
  LangOpts.CPlusPlus = true;
  std::unique_ptr<CFG> &cfg = manager->getCFG(entryFunc);
  cfg->dump(LangOpts, true);
}

void CharArrayBound::getEntryFunc() {
  std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
  for (auto fun : topLevelFuncs) {
    const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
    if (funDecl->isMain()) {
      entryFunc = fun;
      return;
    }
  }
  entryFunc = nullptr;
  return;
}
