#include "framework/BasicChecker.h"

#include <clang/AST/AST.h>
#include <clang/AST/ASTConsumer.h>
#include <clang/AST/Attr.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Analysis/CFG.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/ASTConsumers.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Lex/Lexer.h>
#include <clang/Rewrite/Core/Rewriter.h>
#include <clang/Tooling/CommonOptionsParser.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/Support/raw_ostream.h>

#include <cstdint>
#include <memory>
#include <string>
#include <map>

using namespace clang;

  enum { ERROR, WARNING };
  enum VarType {
    V_INT,
    V_UNSIGNED_INT,
    V_FLOAT,
    V_UNSIGNED_FLOAT,
    V_CHAR,
    V_CHAR_ARRAY,
    UNKNOWN
  };

enum canCheck{
  CAN = 5, CANT
};

  struct VarValue {
    VarDecl *var = nullptr;
    VarType var_type = UNKNOWN;
    bool isDefined = false;
    int layer = 0;
    canCheck ck;
    std::set<float> PosValue;
    std::shared_ptr<void> values;
  };

class Analyzer {
public:

  std::map<std::string, std::set<float>> fun_ret_vals;
  void print_fun_vals(){
    std::map<std::string, std::set<float>>::iterator iter = fun_ret_vals.begin();
    for(;iter!=fun_ret_vals.end();iter++){
      std::cout << "function:" << iter->first << std::endl;
      std::cout << "Possible return values:";
      for(auto i:iter->second){
        std::cout << i << " ";
      }
      std::cout << std::endl;
    }
  }

  std::string funname;
  void setFunName(std::string name){funname = name;}
  bool istestZero(){ return funname == "testZero1"; }

  int try_layer;
  std::vector<std::pair<int, std::string>> if_layer;
  bool isInIfCond;
  void add_try_layer(){
    try_layer = layer;
  }
  void del_try_layer(){
    try_layer = INT32_MAX;
  }
  int add_if_layer(Expr* expr){
    std::string cname(expr->getStmtClassName());
    int index = -1;
    if(cname == "BinaryOperator"){
      BinaryOperator *E = (BinaryOperator*)expr;
      if(E->getOpcode() == BO_GT || E->getOpcode() == BO_LT || E->getOpcode() == BO_NE){
        VarValue vl = DealRValExpr(E->getLHS());
        VarValue vr = DealRValExpr(E->getRHS());
        if(vl.var != nullptr){
          if(vr.PosValue.size() == 1 && vr.PosValue.find(0) != vr.PosValue.end()){
            std::pair<int, std::string> tmp;
            tmp.first = layer;
            tmp.second = vl.var->getNameAsString();
            index = if_layer.size();
            if_layer.push_back(tmp);
          }
        }
        else if(vr.var != nullptr){
          if(vl.PosValue.size() == 1 && vl.PosValue.find(0) != vl.PosValue.end()){
            std::pair<int, std::string> tmp;
            tmp.first = layer;
            tmp.second = vr.var->getNameAsString();
            index = if_layer.size();
            if_layer.push_back(tmp);
          }
        }
      }
    }
    return index;
  }
  void del_if_layer(){
    std::vector<std::pair<int, std::string>>::iterator iter;
    for( iter = if_layer.begin(); iter != if_layer.end(); ){
      if(iter->first >= layer){
        iter = if_layer.erase(iter);
      }else{
        ++iter;
      }
    }
  }
  void print_white_list(){
    std::cout << "try layer: " << try_layer << std::endl;
    for(auto i:if_layer){
      std::cout << "variable " << i.second << " in layer " << i.first << std::endl;
    }
  }
  bool check_white_list(Expr* expr){
    if(layer >= try_layer){
      return true;
    }
    VarValue tmp = DealRValExpr(expr);
    if(tmp.var == nullptr) return false;
    std::string cname(tmp.var->getNameAsString());
    for(auto i:if_layer){
      if(cname == i.second && layer >= i.first){
        return true;
      }
    }
    return false;
  }

  int layer;//记录嵌套层数，用于除模0的安全检测与局部变量增删
  void bindZeroChecker(BasicChecker *checker) { zeroChecker = checker; }
  void bindCharArrayChecker(BasicChecker *checker) {
    charArrayChecker = checker;
  }

  void printValueList() {
    for (auto i : ValueList) {
      std::cout << "possible value of :" << i.var->getNameAsString()
                << std::endl;
      for (auto j : i.PosValue) {
        std::cout << j << " ";
      }
      if (i.var_type == V_CHAR_ARRAY) {
        auto values = std::static_pointer_cast<StrSet>(i.values);
        for (auto &&s : *values) {
          std::cout << s << " ";
        }
      }
      std::cout << std::endl;
    }
  }

  void DealFunctionDecl(const FunctionDecl *fde);
  void DealReturnStmt(ReturnStmt *rtst);
  void DealStmt(Stmt *stmt);
  void DealVarDecl(VarDecl *var);
  void DealCompoundStmt(Stmt *stmt);
  void DealIfStmt(IfStmt *ist);
  void DealSwitchStmt(SwitchStmt *sst);
  void DealForStmt(ForStmt *fst);
  void DealWhileStmt(WhileStmt *wst);
  void DealDoStmt(DoStmt *dst);
  void DealCXXTryStmt(CXXTryStmt *tyst);
  void DealCXXThrowExpr(CXXThrowExpr *trst);
  void DealCXXCatchStmt(CXXTryStmt *cast);
  void IncLayer();
  void DecLayer();

private:
  using UIntSet = std::set<uint64_t>;
  using StrSet = std::set<std::string>;

  std::vector<VarValue> ValueList;
  BasicChecker *zeroChecker = nullptr;
  BasicChecker *charArrayChecker = nullptr;

  void report(BinaryOperator *E, const VarValue &var) {
    if (zeroChecker != nullptr) {
      zeroChecker->report(E, var.PosValue.size() > 1 ? WARNING : ERROR);
    }
  }

  void reportw(BinaryOperator *E) {
    if (zeroChecker != nullptr) {
      zeroChecker->report(E, WARNING);
    }
  }

   void reporte(BinaryOperator *E) {
    if (zeroChecker != nullptr) {
      zeroChecker->report(E, ERROR);
    }
  }

  void report(ArraySubscriptExpr *E, const VarValue &var) {
    if (charArrayChecker != nullptr) {
      charArrayChecker->report(
          E, std::static_pointer_cast<StrSet>(var.values)->size() > 1 ? WARNING
                                                                      : ERROR);
    }
  }

  bool judgeConsist(VarValue v1, VarValue v2) {
    if (v2.PosValue.size() == 0) {
      return false;
    }
    for (auto i : v2.PosValue) {
      if (v1.PosValue.find(i) == v1.PosValue.end()) {
        return false;
      }
    }
    return true;
  }

  int FindVarInList(VarDecl *waitToFind) {
    if (waitToFind == nullptr)
      return -1;
    for (int i = 0; i < ValueList.size(); i++) {
      if (waitToFind->getID() ==
          ValueList[i].var->getID()) {
        return i;
      }
    }
    return -1;
  }

  VarValue DealRValExpr(Expr *expr);
  VarValue DealBinaryOperator(BinaryOperator *E);
  VarValue DealUnaryOperator(UnaryOperator *E);
  VarValue DealConditionalOperator(ConditionalOperator *E);
  VarValue DealDivOp(VarValue v1, VarValue v2, BinaryOperator *E);
  VarValue DealModOp(VarValue v1, VarValue v2, BinaryOperator *E);
  VarValue DealArraySubscriptExpr(ArraySubscriptExpr *expr);
  VarValue DealCallExpr(CallExpr *E);

  VarValue DealAddOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert(i + j);
      }
    }
    return v3;
  }
  VarValue DealSubOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert(i - j);
      }
    }
    return v3;
  }
  VarValue DealMulOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert(i * j);
      }
    }
    return v3;
  }

  VarValue DealAndOp(VarValue v1, VarValue v2){
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert(((int32_t)i) & ((int32_t)j));
      }
    }
    return v3;
  }
  VarValue DealOrOp(VarValue v1, VarValue v2){
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert(((int32_t)i) | ((int32_t)j));
      }
    }
    return v3;
  }
  VarValue DealXorOp(VarValue v1, VarValue v2){
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert(((int32_t)i) ^ ((int32_t)j));
      }
    }
    return v3;
  }

  VarValue DealShlOp(VarValue v1, VarValue v2){
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert(((int32_t)i) << ((int32_t)j));
      }
    }
    return v3;
  }
  VarValue DealShrOp(VarValue v1, VarValue v2){
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert(((uint32_t)i) >> ((uint32_t)j));
      }
    }
    return v3;
  }

  VarValue DealLogAndOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i && j));
      }
    }
    return v3;
  }
  VarValue DealLogOrOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i || j));
      }
    }
    return v3;
  }

  VarValue DealGTOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i > j));
      }
    }
    return v3;
  }
  VarValue DealGEOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i >= j));
      }
    }
    return v3;
  }
  VarValue DealLTOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i < j));
      }
    }
    return v3;
  }
  VarValue DealLEOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i <= j));
      }
    }
    return v3;
  }
  VarValue DealEQOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i == j));
      }
    }
    return v3;
  }
  VarValue DealNEOp(VarValue v1, VarValue v2) {
    VarValue v3;
    for (auto i : v1.PosValue) {
      for (auto j : v2.PosValue) {
        v3.PosValue.insert((i != j));
      }
    }
    return v3;
  }

  VarValue DealPostInc(VarValue v) {
    VarValue PosResult;
    int pos = FindVarInList(v.var);
    if (pos == -1) {
      return PosResult;
    }
    for (auto i : v.PosValue) {
      PosResult.PosValue.insert(++i);
    }
    ValueList[pos].PosValue = PosResult.PosValue;
    return v;
  }
  VarValue DealPostDec(VarValue v) {
    VarValue PosResult;
    int pos = FindVarInList(v.var);
    if (pos == -1) {
      return PosResult;
    }
    for (auto i : v.PosValue) {
      PosResult.PosValue.insert(--i);
    }
    ValueList[pos].PosValue = PosResult.PosValue;
    return v;
  }
  VarValue DealPreInc(VarValue v) {
    VarValue PosResult;
    int pos = FindVarInList(v.var);
    if (pos == -1) {
      return PosResult;
    }
    for (auto i : v.PosValue) {
      PosResult.PosValue.insert(++i);
    }
    ValueList[pos].PosValue = PosResult.PosValue;
    return PosResult;
  }
  VarValue DealPreDec(VarValue v) {
    VarValue PosResult;
    int pos = FindVarInList(v.var);
    if (pos == -1) {
      return PosResult;
    }
    for (auto i : v.PosValue) {
      PosResult.PosValue.insert(--i);
    }
    ValueList[pos].PosValue = PosResult.PosValue;
    return PosResult;
  }
  VarValue DealPlus(VarValue v) { return v; }
  VarValue DealMinus(VarValue v) {
    VarValue PosResult;
    for (auto i : v.PosValue) {
      PosResult.PosValue.insert(-i);
    }
    return PosResult;
  }
  VarValue DealNot(VarValue v){
    VarValue PosResult;
    for (auto i : v.PosValue) {
      PosResult.PosValue.insert(~((int32_t)i));
    }
    return PosResult;
  }
  VarValue DealLogNot(VarValue v){
    VarValue PosResult;
    for (auto i : v.PosValue) {
      PosResult.PosValue.insert(!i);
    }
    return PosResult;
  }
};
