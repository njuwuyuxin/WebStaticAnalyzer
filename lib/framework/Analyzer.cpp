#include "framework/Analyzer.h"
// #include <bits/stdint-uintn.h>
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
//用于头文件中定义的层数的增减
//层数在减少时，应该判断是否有将要消亡的局部变量（层数大于等于当前层数），并将其从变量表中删除

//dealfuncitondecl：处理函数定义
 void Analyzer::DealFunctionDecl(const FunctionDecl *fde){
   funDecl = fde;
   ValueList.clear();
  VarValue Posval;
  layer = 0;
  try_layer = INT32_MAX;
  if_layer.clear();
  unsigned parnum = fde->getNumParams();
  for(unsigned i = 0; i < parnum; i++){
    ParmVarDecl *pv = const_cast<ParmVarDecl*>(fde->getParamDecl(i));
    VarValue new_pv;
    new_pv.var = pv;
    new_pv.layer = layer;
    if(pv->hasDefaultArg()){
      VarValue tmp = DealRValExpr(pv->getDefaultArg());
      for(auto j:tmp.PosValue){
        new_pv.PosValue.insert(j);
        new_pv.isDefined = true;
      }
    }
    else{
      new_pv.PosValue.insert(0);
      new_pv.ck = CANT;
    }
    ValueList.push_back(new_pv);
  }
  //为了提高程序运行效率与简便，将没有默认参数的参数设置为0并加入变量表中
  DealStmt(fde->getBody());
  if(fun_ret_vals.find(funname) == fun_ret_vals.end()){
    fun_ret_vals[funname].insert(0);
  }
  //没有出现return语句，则将0作为默认值传入
  layer = 0;
  try_layer = INT32_MAX;
  if_layer.clear();
  ValueList.clear();
}

//dealreturnstmt：处理return语句
void Analyzer::DealReturnStmt(ReturnStmt *rtst){
  Expr *rt_val = rtst->getRetValue();
  if(rt_val != nullptr){
    VarValue tmp = DealRValExpr(rt_val);
    for(auto i:tmp.PosValue){
      fun_ret_vals[funname].insert(i);
    }
  }
  //调用数值计算函数处理返回值，并将结果传入函数返回值储存表中
}

//dealstmt：对单个stmt进行分类并调用下层函数处理
void Analyzer::DealStmt(Stmt *stmt) {
  if (stmt == nullptr)
    return;
  auto cname = stmt->getStmtClass();
  switch(cname){
  case Stmt::DeclStmtClass: {
    DeclStmt *decltmp = (DeclStmt *)stmt;
    if (decltmp->isSingleDecl()) {
      if(decltmp->getSingleDecl()->getKind() == clang::Decl::Kind::Var){
        DealVarDecl((VarDecl *)(decltmp->getSingleDecl()));
      }
    }
    else {
      auto decls = decltmp->getDeclGroup();
      DeclGroupRef::iterator iter = decls.begin();
      for(;iter!=decls.end();iter++){
        if((*iter)->getKind() == clang::Decl::Kind::Var){
          DealVarDecl((VarDecl *)(*iter));
        }
      }
    }
    //declstmt为变量定义，这里分别对单定义与多定义进行处理
  }break; 
  case Stmt::BinaryOperatorClass: case Stmt::CompoundAssignOperatorClass: {
    BinaryOperator *bopt = (BinaryOperator *)stmt;
    DealBinaryOperator(bopt);
  }break;
  case Stmt::UnaryOperatorClass: {
    UnaryOperator *uopt = (UnaryOperator *)stmt;
    DealUnaryOperator(uopt);
  }break;
  case Stmt::IfStmtClass: {
    IfStmt *ist = (IfStmt *)stmt;
    DealIfStmt(ist);
  }break;
  case Stmt::CompoundStmtClass: {
    CompoundStmt *cst = (CompoundStmt *)stmt;
    DealCompoundStmt(cst);
  }break;
  case Stmt::SwitchStmtClass: {
    SwitchStmt *sst = (SwitchStmt *)stmt;
    DealSwitchStmt(sst);
  }break;
  case Stmt::ForStmtClass: {
    ForStmt *fst = (ForStmt *)stmt;
    DealForStmt(fst);
  }break;
  case Stmt::WhileStmtClass: {
    WhileStmt *wst = (WhileStmt *)stmt;
    DealWhileStmt(wst);
  }break;
  case Stmt::DoStmtClass: {
    DoStmt *dst = (DoStmt *)stmt;
    DealDoStmt(dst);
  }break;
  case Stmt::CXXTryStmtClass: {
    CXXTryStmt *tyst = (CXXTryStmt *)stmt;
    DealCXXTryStmt(tyst);
  }break;
  case Stmt::ReturnStmtClass:{
    ReturnStmt* rtst = (ReturnStmt*)stmt;
    DealReturnStmt(rtst);
  }break;
  default:break;
  }
}

//dealvardecl：对变量定义进行处理
void Analyzer::DealVarDecl(VarDecl *var) {
  VarValue new_var;
  new_var.var = var;
  new_var.layer = layer;
  if (var->hasInit()) {
    new_var.isDefined = true; 
    auto tmp = (DealRValExpr(var->getInit()));
    //取得其初始化表达式，并调用下层函数获得可能值
    new_var.PosValue = tmp.PosValue;
    new_var.values = tmp.values;
    new_var.var_type = tmp.var_type;
  } else {
    new_var.isDefined = false;
    new_var.PosValue.insert(0);
  }
  ValueList.push_back(new_var);
  //将变量初始化结果插入变量状态表中
}

//dealifstmt：处理if语句块
void Analyzer::DealIfStmt(IfStmt *ist) {
  IncLayer();
  VarDecl *new_var = ist->getConditionVariable();
  if (new_var != nullptr) {
    DealVarDecl(new_var);
  }
  //先获取是否有在条件语句里定义的变量
  add_if_layer(ist->getCond());
  VarValue condres = DealRValExpr(ist->getCond());
  //获得的结果condres有三种情况：
  //1、可能取值中有且只有0，这时只需要继续分析else语句块即可
  //2、可能取值中有0也有其他值，这时需要分别以当前的变量状态分析then和else，最后合并两者的结果
  //3、可能取值中没有0，这是只需要继续分析then语句块即可
  if (condres.PosValue.find(0) != condres.PosValue.end()) {
    if (condres.PosValue.size() == 1) {
      //可能取值只有0
      Stmt *elst = ist->getElse();
      if (elst != nullptr) {
        DealCompoundStmt(elst);
      }
    } else {
      //可能取值有0也有不为0，需要先存储当前的变量状态表，再分别分析两种情况，最后合并
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
        for(int n=0; n<ValueList.size(); n++) {
          if (ValueList[n].var->getNameAsString() == i.var->getNameAsString()) {
            new_var = false;
            if (!i.isDefined)
              break;
            for (auto m : i.PosValue) {
              ValueList[n].PosValue.insert(m);
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
    //可能取值不为0，只需要分析then语句块
    Stmt *thst = ist->getThen();
    if (thst != nullptr) {
      DealCompoundStmt(thst);
    }
  }
  del_if_layer();
  DecLayer();
}

//dealcompounstmt：处理大括号语句（块）
void Analyzer::DealCompoundStmt(Stmt *stmt) {
  IncLayer();
  auto cname = stmt->getStmtClass();
  if (cname == Stmt::CompoundStmtClass) {
    CompoundStmt *cst = (CompoundStmt *)stmt;
    for (auto it = cst->body_begin(); it != cst->body_end(); it++) {
      DealStmt(*it);
    }
  } else {
    DealStmt(stmt);
  }
  DecLayer();
}

//dealswitchstmt：处理switch语句块
void Analyzer::DealSwitchStmt(SwitchStmt *sst) {
  IncLayer();
  VarValue condvar = DealRValExpr(sst->getCond());
  //首先，获取switch的条件语句所有可能的取值
  SwitchCase *case_begin = sst->getSwitchCaseList();
  vector<VarValue> stored;
  bool canVisit = false;
  //canVisit记录当前case是否访问
  DefaultStmt *dest = nullptr;
  while (case_begin != nullptr) {
    //依次遍历每一个case或default语句
    auto case_name = case_begin->getStmtClass();
    stored = ValueList;
    if (case_name == Stmt::CaseStmtClass) {
      CaseStmt *cast = (CaseStmt *)case_begin;
      canVisit = judgeConsist(condvar, DealRValExpr(cast->getLHS()));
      //判断当前case 是否符合条件语句的取值，如果符合，直接进入子语句块处理
      if (canVisit) {
        Stmt *sub = cast->getSubStmt();
        auto subname = sub->getStmtClass();
        while (subname == Stmt::CaseStmtClass) {
          //可能有多个case调用同一个子语句块，此处用于处理多个case的情况
          sub = ((CaseStmt *)sub)->getSubStmt();
          subname = sub->getStmtClass();
        }
        DealCompoundStmt(sub);
      }
    } else if (case_name == Stmt::DefaultStmtClass) {
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
    //如果访问过当前case，则将变量取值合并，之后退出（为了方便处理，默认只有一种case符合条件）
  }
  //如果没有任何一种case符合，则调用之前存储的defaultstmt语句块进行分析
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

//循环检测：为了简便处理，循环语句块都视为顺序语句块进行分析
//dealforstmt：处理for语句块，与if类似，判断是否需要分析for的子语句块即可
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

//dealwhilestmt：处理while语句块，与if类似，取条件判断是否需要分析while的子语句块即可
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

//dealdostmt：由于do语句块至少执行一遍，当作顺序语句直接分析子语句块即可
void Analyzer::DealDoStmt(DoStmt *dst) { 
  DealCompoundStmt(dst->getBody()); 
}

//dealcxxtrystmt：处理try语句块
void Analyzer::DealCXXTryStmt(CXXTryStmt *tyst){
  IncLayer();
  add_try_layer();
  // 记录当前层数，在try语句块内发生的除0或模0错误将不会报告
  CompoundStmt *cst = tyst->getTryBlock();
  for (auto it = cst->body_begin(); it != cst->body_end(); it++) {
    auto cname = (*it)->getStmtClass();
    if(cname == Stmt::CXXThrowExprClass){
      break;
    }
    else{
      DealStmt(*it);
    }
  }
  //依次处理子语句块每个stmt，当捕捉到throw语句时，停止并退出
  del_try_layer();
  DecLayer();
}

//dealrvalexpr：处理expr表达式，与dealstmt类似，通过递归调用子函数得到返回值
auto Analyzer::DealRValExpr(Expr *expr) -> VarValue {
  VarValue PosResult;
  auto stmt_class = expr->getStmtClass();
  if (stmt_class == Stmt::ParenExprClass) {
    PosResult = DealRValExpr(((ParenExpr *)expr)->getSubExpr());
    return PosResult;
  } else if (stmt_class == Stmt::ConstantExprClass) {
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
    return PosResult;
  } 
  //处理整形常量
  else if (stmt_class == Stmt::IntegerLiteralClass) {
    PosResult.var_type = V_INT;
    PosResult.PosValue.insert(
        ((IntegerLiteral *)expr)->getValue().getLimitedValue());
    return PosResult;
  } 
  //处理浮点型常量
  else if (stmt_class == Stmt::FloatingLiteralClass) {
    PosResult.var_type = V_FLOAT;
    PosResult.PosValue.insert(
        ((FloatingLiteral *)expr)->getValue().convertToDouble());
    return PosResult;
  } 
  //处理字符型常量
  else if (stmt_class == Stmt::CharacterLiteralClass) {
    PosResult.var_type = V_CHAR;
    PosResult.values = make_shared<UIntSet>(
        initializer_list<uint64_t>{((CharacterLiteral *)expr)->getValue()});
    return PosResult;
  } 
  //处理字符串型常量
  else if (stmt_class == Stmt::StringLiteralClass){
    PosResult.var_type = V_CHAR_ARRAY;
    if(((StringLiteral *)expr)->isUTF8()){
      string str(((StringLiteral *)expr)->getString());
      PosResult.values = make_shared<StrSet>(initializer_list<string>{str});
    }
    else{
      string str(((StringLiteral *)expr)->getLength(), '\0');
      PosResult.values = make_shared<StrSet>(initializer_list<string>{str});
    }
    return PosResult;
    //前面的class为一些固定值的语句，直接调用clang方法得到值并返回即可
  } 
  else if (stmt_class == Stmt::ImplicitCastExprClass) {
    //隐式转换语句，在本分析中仅考虑变量从左值转换为右值，以及单目操作符与该语句块结合的情况
    auto sub_class =
        (((ImplicitCastExpr *)expr)->getSubExpr())->getStmtClass();
    if (sub_class == Stmt::DeclRefExprClass) {
      DeclRefExpr *sub =
          (DeclRefExpr *)(((ImplicitCastExpr *)expr)->getSubExpr());
      int pos = FindVarInList((VarDecl *)(sub->getDecl()));
      if (pos != -1) {
        return ValueList[pos];
      } else {
        PosResult.PosValue.insert(0);
        PosResult.ck = CANT;
      }
      //如果是变量从左值转换为右值，则查找变量状态表并进行相应返回值处理
    } else {
      return DealRValExpr(((ImplicitCastExpr *)expr)->getSubExpr());
    }
    return PosResult;
  } else if (stmt_class == Stmt::DeclRefExprClass) {
    DeclRefExpr *sub = (DeclRefExpr *)expr;
    int pos = FindVarInList((VarDecl *)(sub->getDecl()));
    if (pos != -1) {
      return ValueList[pos];
    } else {
      PosResult.PosValue.insert(0);
      PosResult.ck = CANT;
    }
    return PosResult;
  } 
  //处理双目操作符
  else if (stmt_class == Stmt::BinaryOperatorClass) {
    PosResult = DealBinaryOperator((BinaryOperator *)expr);
    return PosResult;
  } 
  //处理单目操作符
  else if (stmt_class == Stmt::UnaryOperatorClass) {
    PosResult = DealUnaryOperator((UnaryOperator *)expr);
    return PosResult;
  } 
  //处理条件判断操作符（？：）
  else if (stmt_class == Stmt::ConditionalOperatorClass) {
    PosResult = DealConditionalOperator((ConditionalOperator *)expr);
    return PosResult;
  } 
  //处理字符串相关
  else if (stmt_class == Stmt::ArraySubscriptExprClass) {
    PosResult = DealArraySubscriptExpr((ArraySubscriptExpr *)expr);
    return PosResult;
  } 
  //处理函数返回值
  else if (stmt_class == Stmt::CallExprClass){
    PosResult = DealCallExpr((CallExpr*)expr);
    return PosResult;
  }
  if(PosResult.PosValue.size() == 0){
    PosResult.PosValue.insert(0);
    PosResult.ck = CANT;
  }
  return PosResult;
}

//dealbinaryoperator：处理双目操作符
auto Analyzer::DealBinaryOperator(BinaryOperator *E) -> VarValue {
  VarValue PosResult;
  //对双目操作符类型进行判断，并分别调用相应的子方法
  switch (E->getOpcode()) {
  case BO_Assign: {
    auto LClass = E->getLHS()->getStmtClass();
    if (LClass == Stmt::DeclRefExprClass) {
      int pos =
          FindVarInList((VarDecl *)(((DeclRefExpr *)(E->getLHS()))->getDecl()));
      if (pos != -1) {
        ValueList[pos].PosValue = (DealRValExpr(E->getRHS())).PosValue;
        ValueList[pos].isDefined = true;
        ValueList[pos].ck = CAN;
        PosResult.PosValue.insert(1);
      } else {
        PosResult.PosValue.insert(0);
      }
      //等号操作符，需要左边为变量且在变量表里才能正确赋
      //先查找左边变量，之后对右边变量的表达式值进行分析得出取值，之后将结果赋给左边变量
    } else if (LClass == Stmt::ArraySubscriptExprClass) {
      auto arrayExpr = (ArraySubscriptExpr *)(E->getLHS());
      auto var = DealRValExpr(arrayExpr->getBase());
      int pos = FindVarInList(var.var);
      if (pos != -1) {
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
                auto end = s.find('\0');
                if (end != string::npos && i > end) {
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
    } 
      return PosResult;
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
    return DealDivOp(DealRValExpr(E->getLHS()),DealRValExpr(E->getRHS()), E);
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
  } break;
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
  //各种不同的双目操作符，分别调用定义在analyzer.h中各自的处理函数进行处理
  case BO_AddAssign:{
    auto LClass = E->getLHS()->getStmtClass();
    if (LClass == Stmt::DeclRefExprClass) {
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
    auto LClass = E->getLHS()->getStmtClass();
    if (LClass == Stmt::DeclRefExprClass) {
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
    auto LClass = E->getLHS()->getStmtClass();
    if (LClass == Stmt::DeclRefExprClass) {
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
    auto LClass = E->getLHS()->getStmtClass();
    if (LClass == Stmt::DeclRefExprClass) {
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
    auto LClass = E->getLHS()->getStmtClass();
    if (LClass == Stmt::DeclRefExprClass) {
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
    auto LClass = E->getLHS()->getStmtClass();
    if (LClass == Stmt::DeclRefExprClass) {
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
    auto LClass = E->getLHS()->getStmtClass();
    if (LClass == Stmt::DeclRefExprClass) {
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
    auto LClass = E->getLHS()->getStmtClass();
    if (LClass == Stmt::DeclRefExprClass) {
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
    auto LClass = E->getLHS()->getStmtClass();
    if (LClass == Stmt::DeclRefExprClass) {
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
    auto LClass = E->getLHS()->getStmtClass();
    if (LClass == Stmt::DeclRefExprClass) {
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
  //类似+=这样的双目赋值操作符，与=号的处理类似，同时调用头文件中定义的不同符号的处理函数
  default:
    break;
  }
  if(PosResult.PosValue.size() == 0){
    PosResult.PosValue.insert(0);
    PosResult.ck = CANT;
  }
  //本分析无法处理的语句类型，将默认值设置为0并标记无法分析的标签，方便在出现缺陷时报警告
  return PosResult;
}

//dealunaryoperator：处理单目操作符， 与双目操作符类似，先分辨类别再调用子函数处理
auto Analyzer::DealUnaryOperator(UnaryOperator *E) -> VarValue {
  VarValue PosResult = DealRValExpr(E->getSubExpr());
  switch (E->getOpcode()) {
  case UO_PostInc: {
    return DealPostInc(PosResult);
  } break;
  case UO_PostDec: {
    return DealPostDec(PosResult);
  } break;
  case UO_PreInc: {
    return DealPreInc(PosResult);
  } break;
  case UO_PreDec: {
    return DealPreDec(PosResult);
  } break;
  case UO_Plus: {
    return DealPlus(PosResult);
  } break;
  case UO_Minus: {
    return DealMinus(PosResult);
  } break;
  case UO_Not: {
    return DealNot(PosResult);
  } break;
  case UO_LNot: {
    return DealLogNot(PosResult);
  } break;
  default:
    break;
  }
  if(PosResult.PosValue.size() == 0){
    PosResult.PosValue.insert(0);
    PosResult.ck = CANT;
  }
  return PosResult;
}

//dealconditionaloperator：处理?:这样的条件选择操作符
auto Analyzer::DealConditionalOperator(ConditionalOperator *E) -> VarValue {
  VarValue cond = DealRValExpr(E->getCond());
  //获得判断条件的所有可能取值，接下来只需要按照if条件语句那样进行判断即可
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

//dealdiv/modop：除号与模号这两者需要提供警告或错误信息，因此单独处理
auto Analyzer::DealDivOp(VarValue v1, VarValue v2, BinaryOperator *E)
    -> VarValue {
  VarValue v3;
  for (auto i : v1.PosValue) {
    for (auto j : v2.PosValue) {
      if (j == 0) {
        if(!check_white_list(v2)){
          //出现除0错误，判断当前右值是否在try或if的条件表达式里有记录，没有的话就进行报告
          if(v2.ck == CANT){
            //本分析无法处理的语句块，不确定一定为0，报警告
            reportw(E);
          }
          else{
            //根据变量的可能值报信息，如果只有0一个值，报错误；否则报警告
            report(E, v2);
          }
        }
        //为了进行后续运算，除0的结果当作0加入返回值中
        v3.PosValue.insert(0);
      } else {
        v3.PosValue.insert(i / j);
      }
    }
  }
  return v3;
}

//模运算的处理与除运算相似
auto Analyzer::DealModOp(VarValue v1, VarValue v2, BinaryOperator *E)
    -> VarValue {
  VarValue v3;
  for (auto i : v1.PosValue) {
    for (auto j : v2.PosValue) {
      if (j == 0) {
        if(!check_white_list(v2)){
          if(v2.ck == CANT){
            reportw(E);
          }
          else{
            report(E, v2);
          }
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
        auto end = s.find('\0');
        if (end != string::npos && i > end) {
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

//dealcallexpr：函数调用语句的处理，在表中查找是否有当前调用的函数返回值即可
auto Analyzer::DealCallExpr(CallExpr *E) -> VarValue {
  VarValue PosResult;
  const FunctionDecl* funDecl = E->getDirectCallee();
  if(funDecl == nullptr){
    PosResult.PosValue.insert(0);
    PosResult.ck = CANT;
  }
  else{
    std::string cname(funDecl->getNameAsString());
    if(fun_ret_vals.find(cname) != fun_ret_vals.end()){
      for(auto i : fun_ret_vals[cname]){
        PosResult.PosValue.insert(i);
      }
    }
    else{
      PosResult.PosValue.insert(0);
      PosResult.ck = CANT;
    }
  }
  return PosResult;
}