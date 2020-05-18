#include "checkers/CompareChecker.h"

namespace{
  class CompareVisitor : public RecursiveASTVisitor<CompareVisitor>{
  public:
    bool VisitBinaryOperator(BinaryOperator* bop){
      if(bop->isComparisonOp()){
        string Ltype = bop->getLHS()->IgnoreImpCasts()->getType().getAsString();
        string Rtype = bop->getRHS()->IgnoreImpCasts()->getType().getAsString();
        if((find(Signed.begin(), Signed.end(), Ltype) != Signed.end() && find(Unsigned.begin(), Unsigned.end(), Rtype) != Unsigned.end())
        || (find(Signed.begin(), Signed.end(), Rtype) != Signed.end() && find(Unsigned.begin(), Unsigned.end(), Ltype) != Unsigned.end()))
          stmts.push_back(bop);
      }
      return true;
    }

    vector<Stmt*> get_stmt() { return stmts; }
  private:
    static vector<string> Signed;
    static vector<string> Unsigned;
    vector<Stmt*> stmts;
  };

  vector<string> CompareVisitor::Signed = {"short", "int", "long", "long long"};
  vector<string> CompareVisitor::Unsigned = {"unsigned short", "unsigned int", "unsigned long", "unsigned long long"};
}

vector<string> CompareChecker::Signed = {"short", "int", "long", "long long"};
vector<string> CompareChecker::Unsigned = {"unsigned short", "unsigned int", "unsigned long", "unsigned long long"};

void CompareChecker::printStmt(const Stmt* stmt, const SourceManager &sm){
  //string filename = sm.getFilename(stmt->getBeginLoc());
  string begin = stmt->getBeginLoc().printToString(sm);
  cout << begin << endl;
  LangOptions LangOpts;
  LangOpts.CPlusPlus = true;
  stmt->printPretty(outs(), nullptr, LangOpts);
  cout << endl;
}

bool CompareChecker::RecursiveFind(const Stmt* stmt, const ASTContext& context){
  if(stmt != nullptr){
    if(dyn_cast<BinaryOperator>(stmt) && ((BinaryOperator*)stmt)->isComparisonOp()){
      BinaryOperator* bop = (BinaryOperator*)stmt;
      Expr* LHS = bop->getLHS()->IgnoreImpCasts();
      Expr* RHS = bop->getRHS()->IgnoreImpCasts();
      string Ltype = LHS->getType().getAsString();
      string Rtype = RHS->getType().getAsString();
      if((find(Signed.begin(), Signed.end(), Ltype) != Signed.end() && find(Unsigned.begin(), Unsigned.end(), Rtype) != Unsigned.end())
      || (find(Signed.begin(), Signed.end(), Rtype) != Signed.end() && find(Unsigned.begin(), Unsigned.end(), Ltype) != Unsigned.end())){
        printStmt(stmt, context.getSourceManager());
        return true;
      }
    }else{
      for(auto c : stmt->children()){
        if(RecursiveFind(c, context)) break;
      }
    }
  }
  return false;
}

vector<Defect> CompareChecker::check(){
  getEntryFunc();
  vector<Defect> defects;
  if(entryFunc != nullptr){
    FunctionDecl *funDecl = manager->getFunctionDecl(entryFunc);
    Stmt* stmt = funDecl->getBody();
    const ASTContext& context = funDecl->getASTContext();
    CompareVisitor visitor;
    visitor.TraverseStmt(stmt);
    vector<Stmt*> stmts = visitor.get_stmt();
    for(auto s : stmts){
      Defect d;
      d.location = s->getBeginLoc().printToString(context.getSourceManager());
      d.info = "compare statement has both signed and unsigned number";
      defects.push_back(d);
    }
    //RecursiveFind(stmt, context);
  }
  return defects;
}

 /*
    vector<ASTVariable *> variables = entryFunc->getVariables();
    for(auto v: variables){
      VarDecl* varDecl = manager->getVarDecl(v);
      QualType varType = varDecl->getType();
      cout << v->getName() << " " << varType.getAsString() << endl;
    }
    */

void CompareChecker::getEntryFunc() {
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
