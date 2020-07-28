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
  SwitchVisitor(const ASTContext &ctx,const std::vector<EnumDecl*> eds) : ctx(ctx),EDs(eds) {}

  bool VisitSwitchStmt(SwitchStmt *E) {
    Expr* conditionExpr = E->getCond();
    if(conditionExpr!=NULL){
      Decl* cond_decl = conditionExpr->getReferencedDeclOfCallee();
      if(cond_decl==NULL){
        //如果无法得到条件变量的声明语句，说明条件变量为表达式，不需进行后续分析
        return true;
      }
      if(cond_decl->getKind() != clang::Decl::Kind::Var){
        // cout<<"not Var decl"<<endl;
        return true;
      }
      VarDecl* VD = (VarDecl*)(cond_decl);
      string conditionEnumName = VD->getType().getCanonicalType().getAsString();   //获取枚举变量定义中枚举类型的名称
      size_t delim = conditionEnumName.find("enum ");
      if(delim==string::npos){  //not enum type
        return true;
      }
      cout<<"conditionEnumName after="<<conditionEnumName<<endl;
      conditionEnumName = conditionEnumName.substr(5);
      cout<<"conditionEnumName="<<conditionEnumName<<endl;

      for(EnumDecl* ED:EDs){
        string EnumName = ED->getName();
        // cout<<"cur EnumName="<<EnumName<<endl;

        //判断当前switch的条件枚举变量匹配所有枚举类型声明中的哪个
        if(conditionEnumName==EnumName){
          //获取枚举定义中所有元素，存放进enumElements
          std::vector<string> enumElements;
          auto enums = ED->enumerators();
          for(auto iter:enums){
            enumElements.push_back(iter->getNameAsString());
          }
          //遍历每个case
          SwitchCase* c = E->getSwitchCaseList();
          while(c!=NULL){
            auto substmts = c->child_begin()->child_begin()->children();
            //遍历当前case子stmt，找到当前case的expr进行处理
            for(auto substmt: substmts){
              if(DeclRefExpr* case_expr = static_cast<DeclRefExpr*>(substmt)){
                string caseName = case_expr->getDecl()->getNameAsString();
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
          break;
        }
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
  const std::vector<EnumDecl*> EDs;
  std::vector<Defect> defects;
};

void SwitchChecker::check(){
  std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
  std::vector<EnumDecl*> topLevelEnums = resource->getEnums();
  for (auto fun : topLevelFuncs) {
    const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
    auto stmt = funDecl->getBody();
    const ASTContext &ctx = funDecl->getASTContext();
    // stmt->dumpColor();

    SwitchVisitor visitor(ctx,topLevelEnums);
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
