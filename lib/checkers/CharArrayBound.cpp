#include "checkers/CharArrayBound.h"

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");

void CharArrayBound::check() {
  getEntryFunc();
  if (entryFunc != nullptr) {
    auto variables = entryFunc->getVariables();
    for (auto &&v : variables) {
      auto varDecl = manager->getVarDecl(v);
      auto varType = varDecl->getType();
      if (varType.getAsString().find("char ") != string::npos) {
        auto val = varDecl->evaluateValue();
        int len = val->getArrayInitializedElts();
        for (int i = 0; i < len; i++) {
          auto elt = val->getArrayInitializedElt(i).getInt();
          cout << (char)elt.getExtValue();
        }
      }
    }
    cout << endl;
  }
  // std::unique_ptr<CFG> &cfg = manager->getCFG(entryFunc);
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
