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
    vector<Defect> defects;
    const FunctionDecl *funDecl;
public:
  bool VisitBinaryOperator(BinaryOperator *E) {
    if (E->getOpcodeStr() == "/" || E->getOpcodeStr() == "%") {
        string opt = E->getOpcodeStr();
        Expr* ro = E->getRHS()->IgnoreParenCasts()->IgnoreImpCasts();
        Expr::EvalResult rst;
        Expr::ConstExprUsage Usage = Expr::EvaluateForCodeGen;
        if(ro->EvaluateAsConstantExpr(rst, Usage, funDecl->getASTContext())){
          //string begin = ro->getBeginLoc().printToString(funDecl->getASTContext().getSourceManager());
          //cout << begin << endl;
          Defect d;
          auto &sm = funDecl->getASTContext().getSourceManager();
          APValue::ValueKind vtp = rst.Val.getKind();
          switch(vtp){
            case APValue::Int:{
              int64_t val = rst.Val.getInt().getExtValue();
              //cout <<val<<endl;
              if(val == 0){
                d.location = ro->getBeginLoc().printToString(sm);
                d.info = "操作符"+opt+"的右操作数是结果为0的常数表达式";
                defects.push_back(d);
              }
            }break;
            case APValue::Float:{
              float val = rst.Val.getFloat().convertToFloat();
              //cout <<val<<endl;
              if(val == 0){
                d.location = ro->getBeginLoc().printToString(sm);
                d.info = "操作符"+opt+"的右操作数是结果为0的常数表达式";
                defects.push_back(d);
              }
            }break;
            case APValue::FixedPoint:{
              int64_t val = rst.Val.getFixedPoint().getValue().getExtValue();
              //cout <<val<<endl;
              if(val == 0){
                d.location = ro->getBeginLoc().printToString(sm);
                d.info = "操作符"+opt+"的右操作数是结果为0的常数表达式";
                defects.push_back(d);
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

    const vector<Defect> &getDefects() const { return defects; }

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
        auto dfts = visitor.getDefects();
        defects.insert(defects.end(),dfts.begin(),dfts.end());
    }
    return defects;
}

