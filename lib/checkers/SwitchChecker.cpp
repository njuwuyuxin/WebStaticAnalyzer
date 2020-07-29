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
  SwitchVisitor(const ASTContext &ctx,unordered_map<string,EnumDecl*> eds) : ctx(ctx),EDs(eds) {}

  bool VisitSwitchStmt(SwitchStmt *E) {
    cout<<"visit one switch begin!"<<endl;
    Expr* conditionExpr = E->getCond();
    if(conditionExpr!=NULL){
      cout<<"visit one switch"<<endl;
      Decl* cond_decl = conditionExpr->getReferencedDeclOfCallee();
      if(cond_decl==NULL){
        cout<<"can not get cond decl"<<endl;
        //如果无法得到条件变量的声明语句，说明条件变量为表达式，不需进行后续分析
        return true;
      }
      string conditionEnumName;
      if(cond_decl->getKind() == clang::Decl::Kind::Var){
        VarDecl* VD = (VarDecl*)(cond_decl);
        conditionEnumName = VD->getType().getCanonicalType().getAsString();   //获取枚举变量定义中枚举类型的名称
      }
      else if(cond_decl->getKind() == clang::Decl::Kind::ParmVar){
        ParmVarDecl* PVD = (ParmVarDecl*)(cond_decl);
        conditionEnumName = PVD->getType().getCanonicalType().getAsString();   //获取枚举变量定义中枚举类型的名称
      }
      else{
        cout<<"not var decl nor parm var decl"<<endl;
        cond_decl->dumpColor();
        return true;
      }
      size_t delim = conditionEnumName.find("enum ");
      if(delim==string::npos){  //not enum type
        cout<<"not enum type"<<endl;
        cout<<"cur conditionEnumName="<<conditionEnumName<<endl;
        return true;
      }
      cout<<"conditionEnumName after="<<conditionEnumName<<endl;
      conditionEnumName = conditionEnumName.substr(5);
      cout<<"conditionEnumName="<<conditionEnumName<<endl;

      auto ED_iter = EDs.find(conditionEnumName);
      if(ED_iter==EDs.end()){
        cout<<"didn't find ED"<<endl;
        return true;
      }
      cout<<"[SUCCESS]one switch check start"<<endl;
      EnumDecl* ED = ED_iter->second;
      ED->dumpColor();
      //获取枚举定义中所有元素，存放进enumElements
      std::vector<string> enumElements;
      if(ED==NULL){
        cout<<"[ERROR]ED is NULL"<<endl;
        return true;
      }
      auto enums = ED->enumerators();
      
      for(auto iter:enums){
        if(iter==NULL){
          cout<<"cur enum iter is NULL,continue"<<endl;
          continue;
        }
        enumElements.push_back(iter->getNameAsString());
      }
      //遍历每个case
      SwitchCase* c = E->getSwitchCaseList();
      while(c!=NULL){
        auto substmts = c->child_begin()->child_begin()->children();
        //遍历当前case子stmt，找到当前case的expr进行处理
        for(auto substmt: substmts){
          // substmt->dumpColor();
          if(DeclRefExpr* case_expr = dyn_cast<DeclRefExpr>(substmt)){
            ValueDecl* case_decl = case_expr->getDecl();
            if(case_decl==NULL){
              cout<<"case_decl is NULL"<<endl;
              continue;
            }
            string caseName = case_decl->getNameAsString();
            for(auto i=enumElements.begin();i!=enumElements.end();i++){
              if(*i==caseName){
                enumElements.erase(i);
                break;
              }
            }
            break;
          } 
        }
        c=c->getNextSwitchCase();
      }
      //若enumElements非空，则说明有枚举情况尚未覆盖
      if(enumElements.size()!=0){
        string location = E->getBeginLoc().printToString(ctx.getSourceManager());
        string info = "存在尚未覆盖的枚举类型\n";
        for(auto i:enumElements){
          info += i;
          info += " ";
        }
        defects.push_back(make_tuple(location,info));
      }
    }
    
    else{
      cout<<"conditionVar is null"<<endl;
    }
    
    return true;
  }

  std::vector<Defect> getDefects(){
    return defects;
  }

private:
  const ASTContext &ctx;
  const unordered_map<string,EnumDecl*> EDs;
  std::vector<Defect> defects;
};

void SwitchChecker::check(){
  if(call_graph==NULL){
    cout<<"ERROR: call_graph is NULL"<<endl;
    return;
  }
  cout<<"start Switch check"<<endl;
  std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
  cout<<"topLevelFuncs capacity="<<topLevelFuncs.capacity()<<endl;
  unordered_map<string,EnumDecl*> topLevelEnums = resource->getEnums();
  cout<<"enum map capacity="<<topLevelEnums.size()<<endl;
  cout<<"All Top Level Enums:"<<endl;
  for(auto i:topLevelEnums){
    cout<<i.first<<endl;
    i.second->dumpColor();
  }
  cout<<"------------------"<<endl;

  for (auto fun : topLevelFuncs) {
    const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
    auto stmt = funDecl->getBody();
    const ASTContext &ctx = funDecl->getASTContext();
    // stmt->dumpColor();

    SwitchVisitor visitor(ctx,topLevelEnums);
    cout<<"start traverse stmt"<<endl;
    visitor.TraverseStmt(stmt);
    std::vector<Defect> dfs = visitor.getDefects();
    for (auto &&d : dfs) {
      addDefect(move(d));
    }
  }
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
