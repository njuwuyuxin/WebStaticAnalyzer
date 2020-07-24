#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <vector>
#include <queue>

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Analysis/CFG.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/Lexer.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>

#include "framework/BasicChecker.h"

using namespace clang;
using namespace llvm;
using namespace clang::driver;
using namespace clang::tooling;
using namespace std;
/*
enum VarType{
  V_INT, V_UNSIGNED_INT, V_FLOAT, V_UNSIGNED_FLOAT, UNKNOWN
};

struct VarValue{
  VarDecl *var;//variable declaration
  VarType var_type;//variable type
  bool isDefined;//is defined
  set<float> PosValue; //possible value set
  VarValue(){
    var = NULL;
    var_type = UNKNOWN;
    isDefined = false;
  }
};
*/

class ZeroChecker : public BasicChecker {
public:
  ZeroChecker(ASTResource *resource, ASTManager *manager,
                CallGraph *call_graph, Config *configure)
      : BasicChecker(resource, manager, call_graph, configure){};
  void check() override;
  void report(const Expr *expr, int level) override;

private:
  const FunctionDecl *funDecl;
  ASTFunction *entryFunc;
  // vector<VarValue> ValueList;

  void getFunDecl(const FunctionDecl *funDecl){
    this->funDecl = funDecl;
  }
/*
  void defectsClearSamePlace(){
    vector<Defect> tmp;
    for(int i=0;i<defects.size();i++){
      bool hasSame = false;
      for(int j=0;j<tmp.size();j++){
        if(tmp[j].location == defects[i].location){
          hasSame = true;
          break;
        }
      }
      if(!hasSame){
        tmp.push_back(defects[i]);
      }
    }
    defects = tmp;
  }

  void DataFlowAnalysis(CFG* cfg);
  void visitFunctionStmts(Stmt* stmt);
  void VisitCFGBlocks(CFGBlock* block, int &blockid);
  void printValueList(){
    for(auto i:ValueList){
      cout << "possible value of :" << i.var->getNameAsString() << endl;
      for(auto j:i.PosValue){
        cout << j <<  " ";
      }
      cout << endl;
    }
  }

  void DealStmt(Stmt *stmt);
  void DealVarDecl(VarDecl *var);
  int FindVarInList(VarDecl *waitToFind){
    if(waitToFind == nullptr) return -1;
    for(int i=0;i<ValueList.size();i++){
      if(waitToFind->getNameAsString() == ValueList[i].var->getNameAsString()){
        return i;
      }
    }
    return -1;
  }
  void DealCompoundStmt(Stmt *stmt);
  void DealIfStmt(IfStmt *ist);
  void DealSwitchStmt(SwitchStmt *sst);
  void DealForStmt(ForStmt *fst);
  void DealWhileStmt(WhileStmt *wst);
  void DealDoStmt(DoStmt* dst);
  bool judgeConsist(VarValue v1, VarValue v2){
    if(v2.PosValue.size() == 0){
      return false;
    }
    for(auto i:v2.PosValue){
      if(v1.PosValue.find(i)==v1.PosValue.end()){
        return false;
      }
    }
    return true;
  }

  VarValue DealRValExpr(Expr *expr);
  VarValue DealBinaryOperator(BinaryOperator *E);
  VarValue DealUnaryOperator(UnaryOperator *E);
  VarValue DealConditionalOperator(ConditionalOperator *E);

  VarValue DealAddOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert(i+j);
      }
    }
    return v3;
  }
  VarValue DealSubOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert(i-j);
      }
    }
    return v3;
  }
  VarValue DealMulOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert(i*j);
      }
    }
    return v3;
  }
  VarValue DealDivOp(VarValue v1, VarValue v2, BinaryOperator *E){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        if(j == 0){
          Defect d;
          Expr* ro = E->getRHS()->IgnoreParenCasts()->IgnoreImpCasts();
          auto &sm = funDecl->getASTContext().getSourceManager();
          d.location = ro->getBeginLoc().printToString(sm);
          d.info = "warning:操作符'//'的右操作数可能是结果为0的变量表达式";
          defects.push_back(d);
          v3.PosValue.insert(0);
        }
        else{
          v3.PosValue.insert(i/j);
        }
      }
    }
    return v3;
  }
  VarValue DealModOp(VarValue v1, VarValue v2, BinaryOperator *E){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        if(j == 0){
          Defect d;
          Expr* ro = E->getRHS()->IgnoreParenCasts()->IgnoreImpCasts();
          auto &sm = funDecl->getASTContext().getSourceManager();
          d.location = ro->getBeginLoc().printToString(sm);
          d.info = "warning:操作符'%'的右操作数可能是结果为0的变量表达式";
          defects.push_back(d);
          v3.PosValue.insert(0);
        }
        else{
          v3.PosValue.insert(((int)i)%((int)j));
        }
      }
    }
    return v3;
  }

  VarValue DealLogAndOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i&&j));
      }
    }
    return v3;
  }
  VarValue DealLogOrOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i||j));
      }
    }
    return v3;
  }

  VarValue DealGTOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i>j));
      }
    }
    return v3;
  }
  VarValue DealGEOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i>=j));
      }
    }
    return v3;
  }
  VarValue DealLTOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i<j));
      }
    }
    return v3;
  }
  VarValue DealLEOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i<=j));
      }
    }
    return v3;
  }
  VarValue DealEQOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i==j));
      }
    }
    return v3;
  }
  VarValue DealNEOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i!=j));
      }
    }
    return v3;
  }

  VarValue DealPostInc(VarValue v){
    VarValue PosResult;
    int pos = FindVarInList(v.var);
    if(pos == -1){
      return PosResult;
    }
    for(auto i:v.PosValue){
      PosResult.PosValue.insert(++i);
    }
    ValueList[pos].PosValue = PosResult.PosValue;
    return v;
  }
  VarValue DealPostDec(VarValue v){
    VarValue PosResult;
    int pos = FindVarInList(v.var);
    if(pos == -1){
      return PosResult;
    }
    for(auto i:v.PosValue){
      PosResult.PosValue.insert(--i);
    }
    ValueList[pos].PosValue = PosResult.PosValue;
    return v;
  }
  VarValue DealPreInc(VarValue v){
    VarValue PosResult;
    int pos = FindVarInList(v.var);
    if(pos == -1){
      return PosResult;
    }
    for(auto i:v.PosValue){
      PosResult.PosValue.insert(++i);
    }
    ValueList[pos].PosValue = PosResult.PosValue;
    return PosResult;
  }
  VarValue DealPreDec(VarValue v){
    VarValue PosResult;
    int pos = FindVarInList(v.var);
    if(pos == -1){
      return PosResult;
    }
    for(auto i:v.PosValue){
      PosResult.PosValue.insert(--i);
    }
    ValueList[pos].PosValue = PosResult.PosValue;
    return PosResult;
  }
  VarValue DealPlus(VarValue v){
    return v;
  }
  VarValue DealMinus(VarValue v){
    VarValue PosResult;
    for(auto i:v.PosValue){
      PosResult.PosValue.insert(-i);
    }
    return PosResult;
  }*/
};

