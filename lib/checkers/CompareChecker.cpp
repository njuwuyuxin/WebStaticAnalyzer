#include "checkers/CompareChecker.h"

namespace{
  vector<string> Signed = {"short", "int", "long", "long long"};
  vector<string> Unsigned = {"unsigned short", "unsigned int", "unsigned long", "unsigned long long"};

  int isMixedUsing(BinaryOperator *bop){
    if(bop->isComparisonOp()){
        string Ltype = bop->getLHS()->IgnoreImpCasts()->getType().getCanonicalType().getAsString();
        string Rtype = bop->getRHS()->IgnoreImpCasts()->getType().getCanonicalType().getAsString();
        if((find(Signed.begin(), Signed.end(), Ltype) != Signed.end() && find(Unsigned.begin(), Unsigned.end(), Rtype) != Unsigned.end())
        || (find(Signed.begin(), Signed.end(), Rtype) != Signed.end() && find(Unsigned.begin(), Unsigned.end(), Ltype) != Unsigned.end()))
          return 1;
        if(Rtype == "char") swap(Ltype, Rtype);
        if(Ltype == "char" && (find(Signed.begin(), Signed.end(), Rtype) != Signed.end() || find(Unsigned.begin(), Unsigned.end(), Rtype) != Unsigned.end()))
          return 2;
      }
      return 0;
  }

  class CompareVisitor : public RecursiveASTVisitor<CompareVisitor>{
  public:
    bool VisitBinaryOperator(BinaryOperator* bop){
      int index = isMixedUsing(bop);
      if(index == 1)
        errors.push_back(bop);
      if(index == 2)
        warnings.push_back(bop);
      return true;
    }

    vector<Stmt*> get_error() { return errors; }
    vector<Stmt*> get_warning() { return warnings; }
  private:
    vector<Stmt*> errors;
    vector<Stmt*> warnings;
  }; 
}

void CompareChecker::check(){
  //getEntryFunc();
  //vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
  //out of function
  /*
  vector<VarDecl *> vars = resource->getVarDecl();
  for(auto VD : vars){
    CompareVisitor visitor;
    visitor.TraverseStmt(VD->getInit());
    vector<Stmt *> stmts = visitor.get_stmt();
    for(auto s : stmts)
      push_defect(s, VD->getASTContext());
  }*/
  // in function
  vector<ASTFunction *> Funcs = resource->getFunctions();
  for(auto func : Funcs){
    FunctionDecl *funDecl = manager->getFunctionDecl(func);
    Stmt* stmt = funDecl->getBody();
    const ASTContext& context = funDecl->getASTContext();
    CompareVisitor visitor;
    visitor.TraverseStmt(stmt);
    vector<Stmt*> errors = visitor.get_error();
    vector<Stmt*> warnings = visitor.get_warning();
    for(auto e : errors)
      push_defect(e, context);
    for(auto w: warnings)
      push_warning(w, context);
  }
}

void CompareChecker::push_defect(Stmt* s, const ASTContext& context){
  Defect d;
  string location = s->getBeginLoc().printToString(context.getSourceManager());
  string info = "比较运算中混用了有无符号数";
  get<0>(d) = location;
  get<1>(d) = info;
  addDefect(d);
}

void CompareChecker::push_warning(Stmt* s, const ASTContext& context){
  string location = s->getBeginLoc().printToString(context.getSourceManager());
  string info = "warning: 比较运算中可能混用了有无符号数";
  addDefect(make_tuple(location,info));
}

/*
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
*/
