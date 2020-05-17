#include "checkers/CharArrayBound.h"

namespace {

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

vector<Defect> CharArrayBound::check() {
  std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
  vector<Defect> defects;
  for (auto fun : topLevelFuncs) {
    const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
    auto stmt = funDecl->getBody();
    CharArrayVisitor visitor;
    visitor.TraverseStmt(stmt);
    auto stmts = visitor.getStmts();
    auto &sm = funDecl->getASTContext().getSourceManager();
    for (auto &&s : stmts) {
      Defect d;
      d.location = s->getBeginLoc().printToString(sm);
      d.info = "";
      defects.push_back(d);
    }
  }
  return defects;
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
