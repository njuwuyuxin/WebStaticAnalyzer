#include "framework/Common.h"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>

#include <fstream>
#include <iostream>

using namespace std;

namespace {

class ASTFunctionLoad : public ASTConsumer,
                        public RecursiveASTVisitor<ASTFunctionLoad> {
public:
  void HandleTranslationUnit(ASTContext &Context) override {
    TranslationUnitDecl *TUD = Context.getTranslationUnitDecl();
    TraverseDecl(TUD);
  }

  bool TraverseDecl(Decl *D) {
    if (!D)
      return true;
    bool rval = true;

    SourceManager &SM = D->getASTContext().getSourceManager();
    if (SM.isInMainFile(D->getLocation()) ||
        D->getKind() == Decl::TranslationUnit) {
      rval = RecursiveASTVisitor<ASTFunctionLoad>::TraverseDecl(D);
    }
    //排除位于系统头文件（如iostream）中的情况
    else if (!(SM.isInSystemHeader(D->getLocation()) ||
               SM.isInExternCSystemHeader(D->getLocation()) ||
               SM.isInSystemMacro(D->getLocation()))) {
      std::pair<FileID, unsigned> XOffs = SM.getDecomposedLoc(AULoc);
      std::pair<FileID, unsigned> YOffs = SM.getDecomposedLoc(D->getLocation());
      //判断该Decl是否与主文件在同一个TranslateUnit中
      std::pair<bool, bool> InSameTU =
          SM.isInTheSameTranslationUnit(XOffs, YOffs);
      //判断是否位于同一个TranslationUnit中，以及该Decl
      //是否应该被包含于分析过程中
      if (InSameTU.first){
        // cout<<"pass first check"<<endl;
        if(includeOrNot(D)) {
          // cout<<"pass second check"<<endl;
          // cout<<"get in!"<<endl;
          rval = RecursiveASTVisitor<ASTFunctionLoad>::TraverseDecl(D);
        }
      }
    }

    return rval;
  }
  bool TraverseFunctionDecl(FunctionDecl *FD) {
    if (FD && FD->isThisDeclarationADefinition()) {
      functions.push_back(FD);
    }
    return true;
  }
  bool TraverseEnumDecl(EnumDecl *ED) {
    if (ED && ED->isThisDeclarationADefinition()) {
      enums.push_back(ED);
    }
    return true;
  }
  bool TraverseVarDecl(VarDecl *VD){
    if(VD && VD->isThisDeclarationADefinition()){
      vars.push_back(VD);
    }
    return true;
  }

  bool TraverseStmt(Stmt *S) { return true; }
  void setSourceLoc(SourceLocation SL) { AULoc = SL; }


  const std::vector<FunctionDecl *> &getFunctions() const { return functions; }
  const std::vector<EnumDecl *> &getEnums() const { return enums; }
  const std::vector<VarDecl *> &getVarDecl() const { return vars; }

private:
  std::vector<FunctionDecl *> functions;
  std::vector<EnumDecl *> enums;
  std::vector<VarDecl *> vars;
  SourceLocation AULoc;

  bool includeOrNot(const Decl *D) {
    assert(D);
    // 头文件中有自定义类时，形成的AST中其相应的CXXRecordDecl会与主文件代码
    // 存在于同一个TranslationUnit中，但CXXRecordDecl此处会因为hasbody被
    // return
    // false，导致其子节点无法被继续访问，无法获取其在.h文件中的成员函数信息
    // 因此对CXXRecordDecl的节点直接return true以访问类内成员的信息
    if (const CXXRecordDecl *CD = dyn_cast<CXXRecordDecl>(D)) {
      return true;
    }
    if (const EnumDecl *ED = dyn_cast<EnumDecl>(D)) {
      // cout<<"Enum Decl"<<endl;
      return true;
    }
    else{
      return false;
    }

    if (!D->hasBody()){
      cout<<"has no body so false"<<endl;
      return false;
    }

    if (const FunctionDecl *FD = dyn_cast<FunctionDecl>(D)) {
      cout<<"func decl"<<endl;
      // We skip function template definitions, as their semantics is
      // only determined when they are instantiated.
      if (FD->isDependentContext())
        return false;
      // judge if D is inline
      // this part may change in the future
      if (FD->isInlineSpecified()) {
        // return false;
      }
      // AST节点中未发现IdentifierInfo相关信息，且inline函数
      //也没有发现 "__inline" 相关字符串，测试时此条删除暂时没有
      //发现对结果的影响，后续若证实无用可移除下面的代码
      IdentifierInfo *II = FD->getIdentifier();
      if (II && II->getName().startswith("__inline"))
        return false;
    }
    return true;
  }
};

class ASTVariableLoad : public RecursiveASTVisitor<ASTVariableLoad> {
public:
  bool VisitDeclStmt(DeclStmt *S) {
    for (auto D = S->decl_begin(); D != S->decl_end(); D++) {
      if (VarDecl *VD = dyn_cast<VarDecl>(*D)) {
        variables.push_back(VD);
      }
    }
    return true;
  }

  const std::vector<VarDecl *> &getVariables() { return variables; }

private:
  std::vector<VarDecl *> variables;
};

class ASTCalledFunctionLoad
    : public RecursiveASTVisitor<ASTCalledFunctionLoad> {
public:
  bool VisitCallExpr(CallExpr *E) {
    if (FunctionDecl *FD = E->getDirectCallee()) {
      functions.insert(FD);
    }
    return true;
  }

  const std::vector<FunctionDecl *> getFunctions() {
    return std::vector<FunctionDecl *>(functions.begin(), functions.end());
  }

private:
  std::set<FunctionDecl *> functions;
};

class ASTCallExprLoad : public RecursiveASTVisitor<ASTCallExprLoad> {
public:
  bool VisitCallExpr(CallExpr *E) {
    call_exprs.push_back(E);
    return true;
  }

  const std::vector<CallExpr *> getCallExprs() { return call_exprs; }

private:
  std::vector<CallExpr *> call_exprs;
};

} // end of anonymous namespace

namespace common {
/**
 * load an ASTUnit from ast file.
 * AST : the name of the ast file.
 */
std::unique_ptr<ASTUnit> loadFromASTFile(std::string AST) {

  FileSystemOptions FileSystemOpts;
  IntrusiveRefCntPtr<DiagnosticsEngine> Diags =
      CompilerInstance::createDiagnostics(new DiagnosticOptions());
  std::shared_ptr<PCHContainerOperations> PCHContainerOps;
  PCHContainerOps = std::make_shared<PCHContainerOperations>();
  return std::unique_ptr<ASTUnit>(
      ASTUnit::LoadFromASTFile(AST, PCHContainerOps->getRawReader(),
                               ASTUnit::LoadEverything, Diags, FileSystemOpts));
}

/**
 * get all functions's decl from an ast context.
 */
std::vector<FunctionDecl *> getFunctions(ASTContext &Context,SourceLocation SL) {
  ASTFunctionLoad load;
  load.setSourceLoc(SL);
  load.HandleTranslationUnit(Context);
  return load.getFunctions();
}

/**
 * get all enums's decl from an ast context.
 */
std::vector<EnumDecl *> getEnums(ASTContext &Context,SourceLocation SL) {
  ASTFunctionLoad load;
  load.setSourceLoc(SL);
  load.HandleTranslationUnit(Context);
  return load.getEnums();
}

/**
 * get all comparison operator from an ast context.
 */
 std::vector<VarDecl *> getVarDecl(ASTContext &Context){
   ASTFunctionLoad load;
   load.HandleTranslationUnit(Context);
   return load.getVarDecl();
 }

/**
 * get all variables' decl of a function
 * FD : the function decl.
 */
std::vector<VarDecl *> getVariables(FunctionDecl *FD) {
  std::vector<VarDecl *> variables;
  variables.insert(variables.end(), FD->param_begin(), FD->param_end());

  ASTVariableLoad load;
  load.TraverseStmt(FD->getBody());
  variables.insert(variables.end(), load.getVariables().begin(),
                   load.getVariables().end());

  return variables;
}

std::vector<FunctionDecl *> getCalledFunctions(FunctionDecl *FD) {
  ASTCalledFunctionLoad load;
  load.TraverseStmt(FD->getBody());
  return load.getFunctions();
}

std::vector<CallExpr *> getCallExpr(FunctionDecl *FD) {
  ASTCallExprLoad load;
  load.TraverseStmt(FD->getBody());
  return load.getCallExprs();
}

std::string getParams(FunctionDecl *FD) {
  std::string params = "";
  for (auto param = FD->param_begin(); param != FD->param_end(); param++) {
    params = params + (*param)->getOriginalType().getAsString() + "  ";
  }
  return params;
}

std::string getFullName(FunctionDecl *FD) {
  std::string name = FD->getQualifiedNameAsString();

  name = name + "  " + getParams(FD);
  return name;
}
} // end of namespace common

std::string trim(std::string s) {
  std::string result = s;
  result.erase(0, result.find_first_not_of(" \t\r\n"));
  result.erase(result.find_last_not_of(" \t\r\n") + 1);
  return result;
}

std::vector<std::string> initialize(std::string astList) {
  std::vector<std::string> astFiles;

  std::ifstream fin(astList);
  std::string line;
  while (getline(fin, line)) {
    line = trim(line);
    if (line == "")
      continue;
    std::string fileName = line;
    astFiles.push_back(fileName);
  }
  fin.close();

  return astFiles;
}

void common::printLog(std::string logString, common::CheckerName cn, int level,
                      Config &c) {
  auto block = c.getOptionBlock("PrintLog");
  int l = atoi(block.find("level")->second.c_str());
  switch (cn) {
  case common::CheckerName::taintChecker:
    if (block.find("taintChecker")->second == "true" && level >= l) {
      llvm::errs() << logString;
    }
    break;
  case common::CheckerName::danglingPointer:
    if (block.find("TemplateChecker")->second == "true" && level >= l) {
      llvm::errs() << logString;
    }
    break;
  case common::CheckerName::arrayBound:
    if (block.find("arrayBound")->second == "true" && level >= l) {
      llvm::errs() << logString;
    }
    break;
  case common::CheckerName::recursiveCall:
    if (block.find("recursiveCall")->second == "true" && level >= l) {
      llvm::errs() << logString;
    }
    break;
  case common::CheckerName::divideChecker:
    if (block.find("divideChecker")->second == "true" && level >= l) {
      llvm::errs() << logString;
    }
    break;
  case common::CheckerName::memoryOPChecker:
    if (block.find("memoryOPChecker")->second == "true" && level >= l) {
      llvm::errs() << logString;
    }
    break;
  }
}
