#include "checkers/ZeroChecker.h"

namespace {

static inline void printStmt(const Stmt* stmt, const SourceManager &sm){
  string begin = stmt->getBeginLoc().printToString(sm);
  cout << begin << endl;
  LangOptions LangOpts;
  LangOpts.CPlusPlus = true;
  stmt->printPretty(outs(), nullptr, LangOpts);
  cout << endl;
}

class ZeroVisitor : public RecursiveASTVisitor<ZeroVisitor> {
private:
    vector<Stmt *> stmts;
public:
  bool VisitBinaryOperator(BinaryOperator *E) {
    if (E->getOpcodeStr() == "/" || E->getOpcodeStr() == "%") {
        Expr* ro = E->getRHS()->IgnoreParenCasts()->IgnoreImpCasts();
        if(IntegerLiteral* r = dyn_cast<IntegerLiteral>(ro)){
          bool isZero = !(bool)(r->getValue().getLimitedValue());
          if(isZero){
              stmts.push_back(r);
          }
        }
        else if(FloatingLiteral *r = dyn_cast<FloatingLiteral>(ro)){
          bool isZero = r->getValue().isZero();
          if(isZero){
              stmts.push_back(r);
          }
        }
    }
    return true;
    }
    const vector<Stmt *> &getStmts() const { return stmts; }

};
}

vector<Defect> ZeroChecker::check() {
    std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
    vector<Defect> defects;
    for (auto fun : topLevelFuncs) {
        const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
        auto stmt = funDecl->getBody();
        ZeroVisitor visitor;
        visitor.TraverseStmt(stmt);
        auto stmts = visitor.getStmts();
        auto &sm = funDecl->getASTContext().getSourceManager();
        for (auto &&s : stmts) {
          printStmt(s, sm);
          defects.push_back({s->getBeginLoc().printToString(sm)});
        }
    }
    return defects;
}

