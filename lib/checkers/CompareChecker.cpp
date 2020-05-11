#include "checkers/CompareChecker.h"

bool CompareChecker::find(Stmt* stmt, ASTContext& context){
  if(stmt != nullptr){
    if(dyn_cast<BinaryOperator>(stmt) && ((BinaryOperator*)stmt)->isComparisonOp()){
      BinaryOperator* bop = (BinaryOperator*)stmt;
      Expr* lhs = bop->getLHS()->IgnoreImpCasts();
      Expr* rhs = bop->getRHS()->IgnoreImpCasts();
      // DeclRef
      cout << lhs->getType().getAsString() << " " << rhs->getType().getAsString() << endl;
      return true;
    }else{
      for(auto c : stmt->children()){
        if(find(c, context)) break;
      }
    }
  }
  return false;
}

void CompareChecker::check(){
  getEntryFunc();
  if(entryFunc != nullptr){
    FunctionDecl *funDecl = manager->getFunctionDecl(entryFunc);
    //cout << "dump" << endl;
    //funDecl->dump();
    vector<ASTVariable *> variables = entryFunc->getVariables();
    for(auto v: variables){
      VarDecl* varDecl = manager->getVarDecl(v);
      QualType varType = varDecl->getType();
      cout << v->getName() << " " << varType.getAsString() << endl;
    }
    Stmt* stmt = funDecl->getBody();
    ASTContext& context = funDecl->getASTContext();
    find(stmt, context);
    //stmt->dump();
  }
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
