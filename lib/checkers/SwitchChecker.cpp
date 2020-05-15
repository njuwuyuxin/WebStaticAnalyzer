#include "checkers/SwitchChecker.h"

vector<string> SwitchChecker::Signed = {"short", "int", "long", "long long"};
vector<string> SwitchChecker::Unsigned = {"unsigned short", "unsigned int", "unsigned long", "unsigned long long"};

static inline void printStmt(const Stmt* stmt, const SourceManager &sm){
  //string filename = sm.getFilename(stmt->getBeginLoc());

  // string begin = stmt->getBeginLoc().printToString(sm);
  // cout << begin << endl;
  // LangOptions LangOpts;
  // LangOpts.CPlusPlus = true;
  // stmt->printPretty(outs(), nullptr, LangOpts);
  // cout << endl;

  LangOptions LangOpts;
  LangOpts.CPlusPlus = true;
  stmt->getBeginLoc().print(outs(), sm);
  cout << endl;
  stmt->printPretty(outs(), nullptr, LangOpts);
  cout << endl;
}

class SwitchVisitor : public RecursiveASTVisitor<SwitchVisitor> {
public:
  SwitchVisitor(const ASTContext &ctx) : ctx(ctx) {}

  bool VisitSwitchStmt(SwitchStmt *E) {
    // printStmt(E, ctx.getSourceManager());
    Expr* conditionVar = E->getCond();
    if(conditionVar!=NULL){
      // printStmt(conditionVar, ctx.getSourceManager());
      auto cond_decl = conditionVar->getReferencedDeclOfCallee();
      // cond_decl->dump();
      VarDecl* dec = (VarDecl*)(cond_decl);
      // if(val==NULL)
      //   cout<<"val is null"<<endl;
      // else{
      //   val->dump();
      // }
      auto cases = E->getSwitchCaseList();
      
      // cout<<conditionVar->getType().getAsString()<<endl;
      // auto child = conditionVar->child_begin();
      // child->dump();
      // cout<<"condition isn't null!!!"<<endl;
    }
    else{
      cout<<"conditionVar is null"<<endl;
    }
    
    return true;
  }

private:
  const ASTContext &ctx;
};

// bool SwitchChecker::RecursiveFind(const Stmt* stmt, const ASTContext& context){
//   if(stmt != nullptr){
//     if(dyn_cast<BinaryOperator>(stmt) && ((BinaryOperator*)stmt)->isComparisonOp()){
//       BinaryOperator* bop = (BinaryOperator*)stmt;
//       Expr* LHS = bop->getLHS()->IgnoreImpCasts();
//       Expr* RHS = bop->getRHS()->IgnoreImpCasts();
//       string Ltype = LHS->getType().getAsString();
//       string Rtype = RHS->getType().getAsString();
//       if((find(Signed.begin(), Signed.end(), Ltype) != Signed.end() && find(Unsigned.begin(), Unsigned.end(), Rtype) != Unsigned.end())
//       || (find(Signed.begin(), Signed.end(), Rtype) != Signed.end() && find(Unsigned.begin(), Unsigned.end(), Ltype) != Unsigned.end())){
//         printStmt(stmt, context.getSourceManager());
//         return true;
//       }
//     }else{
//       for(auto c : stmt->children()){
//         if(RecursiveFind(c, context)) break;
//       }
//     }
//   }
//   return false;
// }

vector<Defect> SwitchChecker::check(){
  std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
  std::vector<EnumDecl*> topLevelEnums = resource->getEnums();
  for(EnumDecl* ED:topLevelEnums){
    ED->dump();
  }
  for (auto fun : topLevelFuncs) {
    const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
    auto stmt = funDecl->getBody();
    const ASTContext &ctx = funDecl->getASTContext();
    stmt->dumpColor();
    SwitchVisitor visitor(ctx);
    visitor.TraverseStmt(stmt);
  }
  return vector<Defect>();
}

void SwitchChecker::getEntryFunc() {
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
