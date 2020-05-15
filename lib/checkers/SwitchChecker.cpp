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
      TypeDecl* TD = (TypeDecl*)(cond_decl);
      const clang::Type* type = TD->getTypeForDecl();   //获取枚举变量定义中的类型信息
      if(type->isEnumeralType()==false)
        return true;      //如果该switch语句的条件不是枚举类型，则直接返回

      TagType* tag = (TagType*)type;
      string conditionEnumName = tag->getDecl()->getNameAsString();   //获取枚举变量定义中枚举类型的名称
       
      for(EnumDecl* ED:EDs){
        string EnumName = ED->getNameAsString();

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
            Defect df;
            df.location = E->getBeginLoc().printToString(ctx.getSourceManager());
            defects.push_back(df);
            // cout<<E->getBeginLoc().printToString(ctx.getSourceManager())<<endl;
            // cout<<"存在尚未覆盖的枚举类型"<<endl;
            // for(auto i:enumElements){
            //   cout<<i<<" ";
            // }
            // cout<<endl;
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

vector<Defect> SwitchChecker::check(){
  std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
  std::vector<EnumDecl*> topLevelEnums = resource->getEnums();
  std::vector<Defect> defects;
  for (auto fun : topLevelFuncs) {
    const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
    auto stmt = funDecl->getBody();
    const ASTContext &ctx = funDecl->getASTContext();
    // stmt->dumpColor();

    SwitchVisitor visitor(ctx,topLevelEnums);
    visitor.TraverseStmt(stmt);
    std::vector<Defect> dfs = visitor.getDefects();
    defects.insert(defects.end(),dfs.begin(),dfs.end());
  }
  return defects;
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
