#include "checkers/TemplateChecker.h"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

void TemplateChecker::check() {
  readConfig();
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
  std::unique_ptr<CFG>& cfg = manager->getCFG(entryFunc);
  cfg->dump(LangOpts, true);

}

void TemplateChecker::readConfig() {
  std::unordered_map<std::string, std::string> ptrConfig =
      configure->getOptionBlock("TemplateChecker");
  request_fun = stoi(ptrConfig.find("request_fun")->second);
  maxPathInFun = 10;
}

void TemplateChecker::getEntryFunc() {
  std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
  for (auto fun : topLevelFuncs) {
    const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
    if (funDecl->getQualifiedNameAsString() == "main") {
      entryFunc = fun;
      return;
    }
  }
  entryFunc = nullptr;
  return;
}
