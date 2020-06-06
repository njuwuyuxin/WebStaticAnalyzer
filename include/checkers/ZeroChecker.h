#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

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

enum VarType{
  V_INT, V_UNSIGNED_INT, V_FLOAT, V_UNSIGNED_FLOAT, UNKNOWN
};

struct VarValue{
  VarDecl *var;//variable declaration
  VarType var_type;//variable type
  set<float> PosValue; //possible value set
  VarValue(){
    var = NULL;
    var_type = UNKNOWN;
  }
};


class ZeroChecker : public BasicChecker {
public:
  ZeroChecker(ASTResource *resource, ASTManager *manager,
                CallGraph *call_graph, Config *configure)
      : BasicChecker(resource, manager, call_graph, configure){};
  vector<Defect> check();

private:
  ASTFunction *entryFunc;
  vector<VarValue> ValueList;

  void DataFlowAnalysis(CFG* cfg);
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
  VarValue DealRValExpr(Expr *expr);
  VarValue DealBinaryOperator(BinaryOperator *E);
  VarValue DealUnaryOperator(UnaryOperator *E);

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
  VarValue DealDivOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert(i/j);
      }
    }
    return v3;
  }
  VarValue DealModOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((int)i%(int)j);
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
  }
  VarValue DealGEOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i>=j));
      }
    }
  }
  VarValue DealLTOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i<j));
      }
    }
  }
  VarValue DealLEOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i<=j));
      }
    }
  }
  VarValue DealEQOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i==j));
      }
    }
  }
  VarValue DealNEOp(VarValue v1, VarValue v2){
    VarValue v3;
    for(auto i:v1.PosValue){
      for(auto j:v2.PosValue){
        v3.PosValue.insert((i!=j));
      }
    }
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
  }
};

