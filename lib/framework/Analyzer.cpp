#include "framework/Analyzer.h"
#include <bits/stdint-uintn.h>
#include <initializer_list>
#include <memory>

using namespace std;

void Analyzer::IncLayer(){
  layer++;
}

void Analyzer::DecLayer(){
  std::vector<VarValue>::iterator iter;
  for( iter = ValueList.begin(); iter != ValueList.end(); ){
    if(iter->layer == layer){
      iter = ValueList.erase(iter);
    }else{
      ++iter;
    }
  }
  layer--;
}

 VarValue Analyzer::DealFunctionDecl(const FunctionDecl *fde){
  VarValue Posval;
  layer = 0;
  try_layer = INT32_MAX;
  if_layer.clear();
  DealStmt(fde->getBody());
  layer = 0;
  try_layer = INT32_MAX;
  if_layer.clear();
  return Posval;
  //ValueList.clear();
}

void Analyzer::DealStmt(Stmt *stmt) {
  if (stmt == nullptr)
    return;
  string cname(stmt->getStmtClassName());
  if (cname == "DeclStmt") {
    DeclStmt *decltmp = (DeclStmt *)stmt;
    if (decltmp->isSingleDecl()) {
      DealVarDecl((VarDecl *)(decltmp->getSingleDecl()));
    }
  } else if (cname == "BinaryOperator" || cname == "CompoundAssignOperator") {
    BinaryOperator *bopt = (BinaryOperator *)stmt;
    DealBinaryOperator(bopt);
  } else if (cname == "UnaryOperator") {
    UnaryOperator *uopt = (UnaryOperator *)stmt;
    DealUnaryOperator(uopt);
  } else if (cname == "IfStmt") {
    IfStmt *ist = (IfStmt *)stmt;
    DealIfStmt(ist);
  } else if (cname == "CompoundStmt") {
    CompoundStmt *cst = (CompoundStmt *)stmt;
    DealCompoundStmt(cst);
  } else if (cname == "SwitchStmt") {
    SwitchStmt *sst = (SwitchStmt *)stmt;
    DealSwitchStmt(sst);
  } else if (cname == "ForStmt") {
    ForStmt *fst = (ForStmt *)stmt;
    DealForStmt(fst);
  } else if (cname == "WhileStmt") {
    WhileStmt *wst = (WhileStmt *)stmt;
    DealWhileStmt(wst);
  } else if (cname == "DoStmt") {
    DoStmt *dst = (DoStmt *)stmt;
    DealDoStmt(dst);
  } else if (cname == "CXXTryStmt") {
    CXXTryStmt *tyst = (CXXTryStmt *)stmt;
    DealCXXTryStmt(tyst);
  }
}

void Analyzer::DealVarDecl(VarDecl *var) {
  VarValue new_var;
  new_var.var = var;
  new_var.layer = layer;
  if (var->hasInit()) {
    new_var.isDefined = true;
    var->evaluateValue();
    if (auto value = var->getEvaluatedValue(); value != nullptr) {
      switch (value->getKind()) {
      case APValue::Int: {
        int64_t val = value->getInt().getExtValue();
        new_var.var_type = V_INT;
        new_var.PosValue.insert(val);
      } break;
      case APValue::Float: {
        float val = value->getFloat().convertToDouble();
        new_var.var_type = V_FLOAT;
        new_var.PosValue.insert(val);
      } break;
      case APValue::Array:
        if (var->getType().getAsString().find("char ") != string::npos) {
          new_var.var_type = V_CHAR_ARRAY;
          string str(value->getArraySize(), '\0');
          for (unsigned int i = 0; i < value->getArrayInitializedElts(); ++i) {
            str[i] = value->getArrayInitializedElt(i).getInt().getExtValue();
          }
          new_var.values = make_shared<StrSet>(initializer_list<string>{str});
        }
        break;
      default: {
        new_var.PosValue = (DealRValExpr(var->getInit())).PosValue;
      } break;
      }
    } else {
      // new_var.PosValue = DealRValExpr(var->getInit());
    }
  } else {
    new_var.isDefined = false;
    new_var.PosValue.insert(0);
  }
  ValueList.push_back(new_var);
}

void Analyzer::DealIfStmt(IfStmt *ist) {
  IncLayer();
  VarDecl *new_var = ist->getConditionVariable();
  if (new_var != nullptr) {
    DealVarDecl(new_var);
    return;
  }
  add_if_layer(ist->getCond());
  VarValue condres = DealRValExpr(ist->getCond());
  for (auto i : condres.PosValue) {
  }
  if (condres.PosValue.find(0) != condres.PosValue.end()) {
    if (condres.PosValue.size() == 1) {
      Stmt *elst = ist->getElse();
      if (elst != nullptr) {
        DealCompoundStmt(elst);
      }
    } else {
      vector<VarValue> stored(ValueList);
      Stmt *thst = ist->getThen();
      if (thst != nullptr) {
        DealCompoundStmt(thst);
      }
      vector<VarValue> tmp(stored);
      stored = ValueList;
      ValueList = tmp;
      Stmt *elst = ist->getElse();
      if (elst != nullptr) {
        DealCompoundStmt(elst);
      }
      bool new_var;
      for (auto i : stored) {
        new_var = true;
        for (auto j : ValueList) {
          if (j.var->getNameAsString() == i.var->getNameAsString()) {
            new_var = false;
            if (!i.isDefined)
              break;
            for (auto m : i.PosValue) {
              j.PosValue.insert(m);
            }
            break;
          }
          if (new_var) {
            ValueList.push_back(i);
          }
        }
      }
    }
  } else {
    Stmt *thst = ist->getThen();
    if (thst != nullptr) {
      DealCompoundStmt(thst);
    }
  }
  del_if_layer();
  DecLayer();
}

void Analyzer::DealCompoundStmt(Stmt *stmt) {
  IncLayer();
  string cname(stmt->getStmtClassName());
  if (cname == "CompoundStmt") {
    CompoundStmt *cst = (CompoundStmt *)stmt;
    for (auto it = cst->body_begin(); it != cst->body_end(); it++) {
      DealStmt(*it);
    }
  } else {
    DealStmt(stmt);
  }
  DecLayer();
}

void Analyzer::DealSwitchStmt(SwitchStmt *sst) {
  IncLayer();
  VarValue condvar = DealRValExpr(sst->getCond());
  SwitchCase *case_begin = sst->getSwitchCaseList();
  vector<VarValue> stored;
  bool canVisit = false;
  DefaultStmt *dest = nullptr;
  while (case_begin != nullptr) {
    string case_name(case_begin->getStmtClassName());
    stored = ValueList;
    if (case_name == "CaseStmt") {
      CaseStmt *cast = (CaseStmt *)case_begin;
      canVisit = judgeConsist(condvar, DealRValExpr(cast->getLHS()));
      if (canVisit) {
        Stmt *sub = cast->getSubStmt();
        string subname(sub->getStmtClassName());
        while (subname == "CaseStmt") {
          sub = ((CaseStmt *)sub)->getSubStmt();
          subname = sub->getStmtClassName();
        }
        DealCompoundStmt(sub);
      }
    } else if (case_name == "DefaultStmt") {
      dest = (DefaultStmt *)case_begin;
    }
    case_begin = case_begin->getNextSwitchCase();
    if (canVisit) {
      bool new_var;
      for (auto i : stored) {
        new_var = true;
        for (int j = 0; j < ValueList.size(); j++) {
          if (ValueList[j].var->getNameAsString() == i.var->getNameAsString()) {
            new_var = false;
            if (!i.isDefined)
              break;
            for (auto m : i.PosValue) {
              ValueList[j].PosValue.insert(m);
            }
            break;
          }
        }
        if (new_var) {
          ValueList.push_back(i);
        }
      }
      break;
    }
  }
  if (!canVisit) {
    if (dest == nullptr) {
      return;
    }
    DealCompoundStmt(dest->getSubStmt());
    bool new_var;
    for (auto i : stored) {
      new_var = true;
      for (int j = 0; j < ValueList.size(); j++) {
        if (ValueList[j].var->getNameAsString() == i.var->getNameAsString()) {
          new_var = false;
          if (!i.isDefined)
            break;
          for (auto m : i.PosValue) {
            ValueList[j].PosValue.insert(m);
          }
          break;
        }
      }
      if (new_var) {
        ValueList.push_back(i);
      }
    }
  }
  DecLayer();
}

void Analyzer::DealForStmt(ForStmt *fst) {
  IncLayer();
  DealStmt(fst->getInit());
  VarValue res;
  if (fst->getCond() != nullptr) {
    res = DealRValExpr(fst->getCond());
  } else {
    res.PosValue.insert(1);
  }
  if (res.PosValue.find(0) != res.PosValue.end()) {
    if (res.PosValue.size() == 1) {

    } else {
      DealCompoundStmt(fst->getBody());
      DealStmt(fst->getInc());
    }
  } else {
    DealCompoundStmt(fst->getBody());
    DealStmt(fst->getInc());
  }
  DecLayer();
}

void Analyzer::DealWhileStmt(WhileStmt *wst) {
  IncLayer();
  VarValue res = DealRValExpr(wst->getCond());
  if (res.PosValue.find(0) != res.PosValue.end()) {
    if (res.PosValue.size() == 1) {

    } else {
      DealCompoundStmt(wst->getBody());
    }
  } else {
    DealCompoundStmt(wst->getBody());
  }
  DecLayer();
}

void Analyzer::DealDoStmt(DoStmt *dst) { 
  DealCompoundStmt(dst->getBody()); 
}

void Analyzer::DealCXXTryStmt(CXXTryStmt *tyst){
  IncLayer();
  add_try_layer();
  CompoundStmt *cst = tyst->getTryBlock();
  CXXThrowExpr *trst = nullptr;
  for (auto it = cst->body_begin(); it != cst->body_end(); it++) {
    string cname((*it)->getStmtClassName());
    if(cname == "CXXThrowExpr"){
      trst = (CXXThrowExpr*)(*it);
      break;
    }
    else{
      DealStmt(*it);
    }
  }
  if(trst != nullptr){
    VarValue trv = DealRValExpr(trst->getSubExpr());
  }
  del_try_layer();
  DecLayer();
}

auto Analyzer::DealRValExpr(Expr *expr) -> VarValue {
  string stmt_class = expr->getStmtClassName();
  VarValue PosResult;
  if (stmt_class == "ParenExpr") {
    PosResult = DealRValExpr(((ParenExpr *)expr)->getSubExpr());
  } else if (stmt_class == "ConstantExpr") {
    ConstantExpr *cex = (ConstantExpr *)expr;
    switch (cex->getAPValueResult().getKind()) {
    case APValue::Int: {
      int64_t val = cex->getAPValueResult().getInt().getExtValue();
      PosResult.var_type = V_INT;
      PosResult.PosValue.insert(val);
    } break;
    case APValue::Float: {
      float val = cex->getAPValueResult().getFloat().convertToDouble();
      PosResult.var_type = V_FLOAT;
      PosResult.PosValue.insert(val);
    } break;
    default: {
    } break;
    }
  } else if (stmt_class == "IntegerLiteral") {
    PosResult.var_type = V_INT;
    PosResult.PosValue.insert(
        ((IntegerLiteral *)expr)->getValue().getLimitedValue());
  } else if (stmt_class == "FloatingLiteral") {
    PosResult.var_type = V_FLOAT;
    PosResult.PosValue.insert(
        ((FloatingLiteral *)expr)->getValue().convertToDouble());
  } else if (stmt_class == "CharacterLiteral") {
    PosResult.var_type = V_CHAR;
    PosResult.values = make_shared<UIntSet>(
        initializer_list<uint64_t>{((CharacterLiteral *)expr)->getValue()});
  } else if (stmt_class == "ImplicitCastExpr") {
    string sub_class =
        (((ImplicitCastExpr *)expr)->getSubExpr())->getStmtClassName();
    if (sub_class == "DeclRefExpr") {
      DeclRefExpr *sub =
          (DeclRefExpr *)(((ImplicitCastExpr *)expr)->getSubExpr());
      int pos = FindVarInList((VarDecl *)(sub->getDecl()));
      if (pos != -1) {
        return ValueList[pos];
      } else {
        PosResult.PosValue.insert(0);
      }
    } else if (sub_class == "UnaryOperator") {
      PosResult = DealUnaryOperator(
          (UnaryOperator *)(((ImplicitCastExpr *)expr)->getSubExpr()));
    } else {
      return DealRValExpr(((ImplicitCastExpr *)expr)->getSubExpr());
    }
  } else if (stmt_class == "DeclRefExpr") {
    DeclRefExpr *sub = (DeclRefExpr *)expr;
    int pos = FindVarInList((VarDecl *)(sub->getDecl()));
    if (pos != -1) {
      return ValueList[pos];
    } else {
      PosResult.PosValue.insert(0);
    }
  } else if (stmt_class == "BinaryOperator") {
    PosResult = DealBinaryOperator((BinaryOperator *)expr);
  } else if (stmt_class == "UnaryOperator") {
    PosResult = DealUnaryOperator((UnaryOperator *)expr);
  } else if (stmt_class == "ConditionalOperator") {
    PosResult = DealConditionalOperator((ConditionalOperator *)expr);
  } else if (stmt_class == "ArraySubscriptExpr") {
    PosResult = DealArraySubscriptExpr((ArraySubscriptExpr *)expr);
  }
  return PosResult;
}

auto Analyzer::DealBinaryOperator(BinaryOperator *E) -> VarValue {
  VarValue PosResult;
  switch (E->getOpcode()) {
  case BO_Assign: {
    string LClass(E->getLHS()->getStmtClassName());
    if (LClass == "DeclRefExpr") {
      int pos =
          FindVarInList((VarDecl *)(((DeclRefExpr *)(E->getLHS()))->getDecl()));
      if (pos != -1) {
        ValueList[pos].PosValue = (DealRValExpr(E->getRHS())).PosValue;
        ValueList[pos].isDefined = true;
        PosResult.PosValue.insert(1);
      } else {
        PosResult.PosValue.insert(0);
      }
    } else if (LClass == "ArraySubscriptExpr") {
      auto arrayExpr = (ArraySubscriptExpr *)E->getLHS();
      auto var = DealRValExpr(arrayExpr->getBase());
      if (int pos = FindVarInList(var.var); pos != -1) {
        auto idx = DealRValExpr(arrayExpr->getIdx());
        auto rhs = DealRValExpr(E->getRHS());
        if (idx.PosValue.empty() || rhs.values == nullptr) {
          report(arrayExpr, var);
          return PosResult;
        }
        auto rhsvalues = static_pointer_cast<UIntSet>(rhs.values);
        if (var.var_type == V_CHAR_ARRAY) {
          auto values = static_pointer_cast<StrSet>(ValueList[pos].values);
          int len = values->cbegin()->length();
          for (auto &&i : idx.PosValue) {
            if (i >= len) {
              report(arrayExpr, var);
              return PosResult;
            }
            for (auto &&s : *values) {
              for (auto &&c : *rhsvalues) {
                if (auto end = s.find('\0'); end != string::npos && i > end) {
                  report(arrayExpr, var);
                }
                if (s[i] != c) {
                  string ns(s);
                  ns[i] = c;
                  values->insert(move(ns));
                }
              }
            }
          }
        }
      }
    } else {
      return PosResult;
    }
  } break;
  case BO_Add: {
    return DealAddOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  } break;
  case BO_Sub: {
    return DealSubOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  } break;
  case BO_Mul: {
    return DealMulOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  } break;
  case BO_Div: {
    return DealDivOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()), E);
  } break;
  case BO_Rem: {
    return DealModOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()), E);
  } break;
  case BO_And:{
    return DealAndOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  }break;
  case BO_Or:{
    return DealOrOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  }break;
  case BO_Xor:{
    return DealXorOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  }break;
  case BO_LAnd: {
    return DealLogAndOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  } break;
  case BO_LOr: {
    return DealLogOrOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  }
  case BO_Shl:{
    return DealShlOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  }break;
  case BO_Shr:{
    return DealShrOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  }break;
  case BO_LT: {
    return DealLTOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  } break;
  case BO_LE: {
    return DealLEOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  } break;
  case BO_GT: {
    return DealGTOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  } break;
  case BO_GE: {
    return DealGEOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  } break;
  case BO_EQ: {
    return DealEQOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  } break;
  case BO_NE: {
    return DealNEOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  } break;
  case BO_AddAssign:{
    string LClass(E->getLHS()->getStmtClassName());
    if (LClass == "DeclRefExpr") {
      int pos =
          FindVarInList((VarDecl *)(((DeclRefExpr *)(E->getLHS()))->getDecl()));
      if (pos != -1) {
        auto lv = ValueList[pos].PosValue;
        VarValue ltmp;
        ltmp.PosValue = lv;
        ltmp = DealAddOp(ltmp, DealRValExpr(E->getRHS()));
        ValueList[pos].PosValue = ltmp.PosValue;
        ValueList[pos].isDefined = true;
        PosResult.PosValue.insert(1);
      } else {
        PosResult.PosValue.insert(0);
      }
    }
  }break;
  case BO_SubAssign:{
    string LClass(E->getLHS()->getStmtClassName());
    if (LClass == "DeclRefExpr") {
      int pos =
          FindVarInList((VarDecl *)(((DeclRefExpr *)(E->getLHS()))->getDecl()));
      if (pos != -1) {
        auto lv = ValueList[pos].PosValue;
        VarValue ltmp;
        ltmp.PosValue = lv;
        ltmp = DealSubOp(ltmp, DealRValExpr(E->getRHS()));
        ValueList[pos].PosValue = ltmp.PosValue;
        ValueList[pos].isDefined = true;
        PosResult.PosValue.insert(1);
      } else {
        PosResult.PosValue.insert(0);
      }
    }
  }break;
  case BO_MulAssign:{
    string LClass(E->getLHS()->getStmtClassName());
    if (LClass == "DeclRefExpr") {
      int pos =
          FindVarInList((VarDecl *)(((DeclRefExpr *)(E->getLHS()))->getDecl()));
      if (pos != -1) {
        auto lv = ValueList[pos].PosValue;
        VarValue ltmp;
        ltmp.PosValue = lv;
        ltmp = DealMulOp(ltmp, DealRValExpr(E->getRHS()));
        ValueList[pos].PosValue = ltmp.PosValue;
        ValueList[pos].isDefined = true;
        PosResult.PosValue.insert(1);
      } else {
        PosResult.PosValue.insert(0);
      }
    }
  }break;
  case BO_DivAssign:{
    string LClass(E->getLHS()->getStmtClassName());
    if (LClass == "DeclRefExpr") {
      int pos =
          FindVarInList((VarDecl *)(((DeclRefExpr *)(E->getLHS()))->getDecl()));
      if (pos != -1) {
        auto lv = ValueList[pos].PosValue;
        VarValue ltmp;
        ltmp.PosValue = lv;
        ltmp = DealDivOp(ltmp, DealRValExpr(E->getRHS()), E);
        ValueList[pos].PosValue = ltmp.PosValue;
        ValueList[pos].isDefined = true;
        PosResult.PosValue.insert(1);
      } else {
        PosResult.PosValue.insert(0);
      }
    }
  }break;
  case BO_RemAssign:{
    string LClass(E->getLHS()->getStmtClassName());
    if (LClass == "DeclRefExpr") {
      int pos =
          FindVarInList((VarDecl *)(((DeclRefExpr *)(E->getLHS()))->getDecl()));
      if (pos != -1) {
        auto lv = ValueList[pos].PosValue;
        VarValue ltmp;
        ltmp.PosValue = lv;
        ltmp = DealModOp(ltmp, DealRValExpr(E->getRHS()), E);
        ValueList[pos].PosValue = ltmp.PosValue;
        ValueList[pos].isDefined = true;
        PosResult.PosValue.insert(1);
      } else {
        PosResult.PosValue.insert(0);
      }
    }
  }break;
  case BO_AndAssign:{
    string LClass(E->getLHS()->getStmtClassName());
    if (LClass == "DeclRefExpr") {
      int pos =
          FindVarInList((VarDecl *)(((DeclRefExpr *)(E->getLHS()))->getDecl()));
      if (pos != -1) {
        auto lv = ValueList[pos].PosValue;
        VarValue ltmp;
        ltmp.PosValue = lv;
        ltmp = DealAndOp(ltmp, DealRValExpr(E->getRHS()));
        ValueList[pos].PosValue = ltmp.PosValue;
        ValueList[pos].isDefined = true;
        PosResult.PosValue.insert(1);
      } else {
        PosResult.PosValue.insert(0);
      }
    }
  }break;
  case BO_OrAssign:{
    string LClass(E->getLHS()->getStmtClassName());
    if (LClass == "DeclRefExpr") {
      int pos =
          FindVarInList((VarDecl *)(((DeclRefExpr *)(E->getLHS()))->getDecl()));
      if (pos != -1) {
        auto lv = ValueList[pos].PosValue;
        VarValue ltmp;
        ltmp.PosValue = lv;
        ltmp = DealOrOp(ltmp, DealRValExpr(E->getRHS()));
        ValueList[pos].PosValue = ltmp.PosValue;
        ValueList[pos].isDefined = true;
        PosResult.PosValue.insert(1);
      } else {
        PosResult.PosValue.insert(0);
      }
    }
  }break;
  case BO_XorAssign:{
    string LClass(E->getLHS()->getStmtClassName());
    if (LClass == "DeclRefExpr") {
      int pos =
          FindVarInList((VarDecl *)(((DeclRefExpr *)(E->getLHS()))->getDecl()));
      if (pos != -1) {
        auto lv = ValueList[pos].PosValue;
        VarValue ltmp;
        ltmp.PosValue = lv;
        ltmp = DealXorOp(ltmp, DealRValExpr(E->getRHS()));
        ValueList[pos].PosValue = ltmp.PosValue;
        ValueList[pos].isDefined = true;
        PosResult.PosValue.insert(1);
      } else {
        PosResult.PosValue.insert(0);
      }
    }
  }break;
  case BO_ShlAssign:{
    string LClass(E->getLHS()->getStmtClassName());
    if (LClass == "DeclRefExpr") {
      int pos =
          FindVarInList((VarDecl *)(((DeclRefExpr *)(E->getLHS()))->getDecl()));
      if (pos != -1) {
        auto lv = ValueList[pos].PosValue;
        VarValue ltmp;
        ltmp.PosValue = lv;
        ltmp = DealShlOp(ltmp, DealRValExpr(E->getRHS()));
        ValueList[pos].PosValue = ltmp.PosValue;
        ValueList[pos].isDefined = true;
        PosResult.PosValue.insert(1);
      } else {
        PosResult.PosValue.insert(0);
      }
    }
  }break;
  case BO_ShrAssign:{
    string LClass(E->getLHS()->getStmtClassName());
    if (LClass == "DeclRefExpr") {
      int pos =
          FindVarInList((VarDecl *)(((DeclRefExpr *)(E->getLHS()))->getDecl()));
      if (pos != -1) {
        auto lv = ValueList[pos].PosValue;
        VarValue ltmp;
        ltmp.PosValue = lv;
        ltmp = DealShrOp(ltmp, DealRValExpr(E->getRHS()));
        ValueList[pos].PosValue = ltmp.PosValue;
        ValueList[pos].isDefined = true;
        PosResult.PosValue.insert(1);
      } else {
        PosResult.PosValue.insert(0);
      }
    }
  }break;
  default:
    break;
  }
  return PosResult;
}

auto Analyzer::DealUnaryOperator(UnaryOperator *E) -> VarValue {
  VarValue PosResult = DealRValExpr(E->getSubExpr());
  switch (E->getOpcode()) {
  case UO_PostInc: {
    PosResult = DealPostInc(PosResult);
  } break;
  case UO_PostDec: {
    PosResult = DealPostDec(PosResult);
  } break;
  case UO_PreInc: {
    PosResult = DealPreInc(PosResult);
  } break;
  case UO_PreDec: {
    PosResult = DealPreDec(PosResult);
  } break;
  case UO_Plus: {
    PosResult = DealPlus(PosResult);
  } break;
  case UO_Minus: {
    PosResult = DealMinus(PosResult);
  } break;
  case UO_Not: {
    PosResult = DealNot(PosResult);
  } break;
  case UO_LNot: {
    PosResult = DealLogNot(PosResult);
  } break;
  default:
    break;
  }
  return PosResult;
}

auto Analyzer::DealConditionalOperator(ConditionalOperator *E) -> VarValue {
  VarValue cond = DealRValExpr(E->getCond());
  if (cond.PosValue.find(0) != cond.PosValue.end()) {
    if (cond.PosValue.size() == 1) {
      return DealRValExpr(E->getFalseExpr());
    } else {
      VarValue trueres = DealRValExpr(E->getTrueExpr());
      VarValue falseres = DealRValExpr(E->getFalseExpr());
      for (auto i : falseres.PosValue) {
        trueres.PosValue.insert(i);
      }
      return trueres;
    }
  } else {
    return DealRValExpr(E->getTrueExpr());
  }
}

auto Analyzer::DealDivOp(VarValue v1, VarValue v2, BinaryOperator *E)
    -> VarValue {
  VarValue v3;
  for (auto i : v1.PosValue) {
    for (auto j : v2.PosValue) {
      if (j == 0) {
        if(!check_white_list(E->getRHS())){
          report(E, v2);
        }
        v3.PosValue.insert(0);
      } else {
        v3.PosValue.insert(i / j);
      }
    }
  }
  return v3;
}

auto Analyzer::DealModOp(VarValue v1, VarValue v2, BinaryOperator *E)
    -> VarValue {
  VarValue v3;
  for (auto i : v1.PosValue) {
    for (auto j : v2.PosValue) {
      if (j == 0) {
        if(!check_white_list(E->getRHS())){
          report(E, v2);
        }
        v3.PosValue.insert(0);
      } else {
        v3.PosValue.insert(((int)i) % ((int)j));
      }
    }
  }
  return v3;
}

auto Analyzer::DealArraySubscriptExpr(ArraySubscriptExpr *E) -> VarValue {
  VarValue PosResult;
  auto var = DealRValExpr(E->getBase());
  auto idx = DealRValExpr(E->getIdx());
  if (var.var_type == V_CHAR_ARRAY) {
    auto values = static_pointer_cast<StrSet>(var.values);
    auto len = values->cbegin()->length();
    for (auto &&i : idx.PosValue) {
      if (i >= len) {
        report(E, var);
        return PosResult;
      }
      auto p = make_shared<UIntSet>();
      for (auto &&s : *values) {
        if (auto end = s.find('\0'); end != string::npos && i > end) {
          report(E, var);
        }
        p->insert(s[i]);
      }
      PosResult.var_type = V_CHAR;
      PosResult.values = move(p);
    }
  }
  return PosResult;
}
