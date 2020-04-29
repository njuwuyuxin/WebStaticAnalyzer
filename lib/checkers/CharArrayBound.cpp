#include "checkers/CharArrayBound.h"

static inline void printStmt(const Stmt *stmt, const SourceManager &sm) {
  int prefixLen = sm.getFilename(stmt->getBeginLoc()).size() + 1;
  string begin = stmt->getBeginLoc().printToString(sm);
  string end = stmt->getEndLoc().printToString(sm);
  LangOptions LangOpts;
  LangOpts.CPlusPlus = true;
  cout << begin.substr(prefixLen, begin.size() - prefixLen) << ", "
       << end.substr(prefixLen, end.size() - prefixLen) << endl;
  stmt->printPretty(outs(), nullptr, LangOpts);
  cout << endl;
}

static inline bool findVisit(const Stmt *stmt, const ASTContext &ctx) {
  if (stmt != nullptr) {
    if (ArraySubscriptExpr::classof(stmt) &&
        ((Expr *)stmt)->getType().getAsString() == "char") {
      return true;
    } else {
      for (auto &&s : stmt->children()) {
        if (findVisit(s, ctx)) {
          printStmt(stmt, ctx.getSourceManager());
          break;
        }
      }
    }
  }
  return false;
}

void CharArrayBound::check() {
  getEntryFunc();
  if (entryFunc != nullptr) {
    auto variables = entryFunc->getVariables();
    map<string, int> charArrs;
    for (auto &&v : variables) {
      auto varDecl = manager->getVarDecl(v);
      auto varType = varDecl->getType();
      if (varType.getAsString().find("char ") != string::npos) {
        auto val = varDecl->evaluateValue();
        int len = val->getArrayInitializedElts();
        charArrs.insert(make_pair(varDecl->getNameAsString(), len));
      }
    }
    auto funDecl = manager->getFunctionDecl(entryFunc);
    auto stmt = funDecl->getBody();
    const ASTContext &ctx = funDecl->getASTContext();
    findVisit(stmt, ctx);
  }
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
