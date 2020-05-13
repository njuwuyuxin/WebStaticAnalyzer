#include "checkers/CharArrayBound.h"

namespace {

static inline void printStmt(const Stmt *stmt, const SourceManager &sm) {
  LangOptions LangOpts;
  LangOpts.CPlusPlus = true;
  stmt->getBeginLoc().print(outs(), sm);
  cout << endl;
  stmt->printPretty(outs(), nullptr, LangOpts);
  cout << endl;
}

class CharArrayVisitor : public RecursiveASTVisitor<CharArrayVisitor> {
public:
  CharArrayVisitor(const ASTContext &ctx) : ctx(ctx) {}

  bool VisitArraySubscriptExpr(ArraySubscriptExpr *E) {
    if (E->getType().getAsString() == "char") {
      printStmt(E, ctx.getSourceManager());
    }
    return true;
  }

private:
  const ASTContext &ctx;
};

} // namespace

void CharArrayBound::check() {
  // getEntryFunc();
  // if (entryFunc != nullptr) {
  //   auto variables = entryFunc->getVariables();
  //   map<string, int> charArrs;
  //   for (auto &&v : variables) {
  //     auto varDecl = manager->getVarDecl(v);
  //     auto varType = varDecl->getType();
  //     if (varType.getAsString().find("char ") != string::npos) {
  //       auto val = varDecl->evaluateValue();
  //       int len = val->getArrayInitializedElts();
  //       charArrs.insert(make_pair(varDecl->getNameAsString(), len));
  //     }
  //   }
  // }

  std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
  for (auto fun : topLevelFuncs) {
    const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
    auto stmt = funDecl->getBody();
    const ASTContext &ctx = funDecl->getASTContext();
    CharArrayVisitor visitor(ctx);
    visitor.TraverseStmt(stmt);
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
