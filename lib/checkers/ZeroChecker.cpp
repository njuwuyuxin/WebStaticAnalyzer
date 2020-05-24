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
    const FunctionDecl *funDecl;
public:
  bool VisitBinaryOperator(BinaryOperator *E) {
    if (E->getOpcodeStr() == "/" || E->getOpcodeStr() == "%") {
        Expr* ro = E->getRHS()->IgnoreParenCasts()->IgnoreImpCasts();
        Expr::EvalResult rst;
        Expr::ConstExprUsage Usage = Expr::EvaluateForCodeGen;
        if(ro->EvaluateAsConstantExpr(rst, Usage, funDecl->getASTContext())){
          //string begin = ro->getBeginLoc().printToString(funDecl->getASTContext().getSourceManager());
          //cout << begin << endl;
          APValue::ValueKind vtp = rst.Val.getKind();
          switch(vtp){
            case APValue::Int:{
              int64_t val = rst.Val.getInt().getExtValue();
              //cout <<val<<endl;
              if(val == 0){
                stmts.push_back(ro);
              }
            }break;
            case APValue::Float:{
              float val = rst.Val.getFloat().convertToFloat();
              //cout <<val<<endl;
              if(val == 0){
                stmts.push_back(ro);
              }
            }break;
            case APValue::FixedPoint:{
              int64_t val = rst.Val.getFixedPoint().getValue().getExtValue();
              //cout <<val<<endl;
              if(val == 0){
                stmts.push_back(ro);
              }
            }break;
            default:break;
          }
        }
    }
    return true;
    }

  
  void getFun(const FunctionDecl *funDecl){
    this->funDecl = funDecl;
  }

    const vector<Stmt *> &getStmts() const { return stmts; }

};
}

vector<Defect> ZeroChecker::check() {
    std::vector<ASTFunction *> Funcs = resource->getFunctions();
    vector<Defect> defects;
    for (auto fun : Funcs) {
        const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
        auto stmt = funDecl->getBody();
        ZeroVisitor visitor;
        visitor.getFun(funDecl);
        visitor.TraverseStmt(stmt);
        auto stmts = visitor.getStmts();
        auto &sm = funDecl->getASTContext().getSourceManager();
        for (auto &&s : stmts) {
          Defect d;
          d.location = s->getBeginLoc().printToString(sm);
          d.info = "除号/模号右边出现了自然数0";
          defects.push_back(d);
        }
    }
    return defects;
}

