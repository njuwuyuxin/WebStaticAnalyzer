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
  bool VisitArraySubscriptExpr(ArraySubscriptExpr *E) {
    if (E->getType().getAsString() == "char") {
      const VarDecl *vd = (VarDecl *)E->getBase()->getReferencedDeclOfCallee();
      int len = vd->evaluateValue()->getArrayInitializedElts();
      auto idx = E->getIdx();
      Expr::EvalResult i;
      if (!idx->EvaluateAsInt(i, vd->getASTContext()) || i.Val.getInt() > len) {
        stmts.push_back(E);
      }
    }
    return true;
  }

  const vector<Stmt *> &getStmts() const { return stmts; }

private:
  vector<Stmt *> stmts;
};

} // namespace

void CharArrayBound::check() {
  std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
  for (auto fun : topLevelFuncs) {
    const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
    auto stmt = funDecl->getBody();
    CharArrayVisitor visitor;
    visitor.TraverseStmt(stmt);
    auto stmts = visitor.getStmts();
    auto &sm = funDecl->getASTContext().getSourceManager();
    for (auto &&s : stmts) {
      printStmt(s, sm);
    }
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
