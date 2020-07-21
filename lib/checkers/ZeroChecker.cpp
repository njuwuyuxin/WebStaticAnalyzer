#include "checkers/ZeroChecker.h"
#include "framework/Analyzer.h"

namespace {

static inline void printStmt(const Stmt* stmt, const SourceManager &sm){
  string begin = stmt->getBeginLoc().printToString(sm);
  cout << begin << endl;
  LangOptions LangOpts;
  LangOpts.CPlusPlus = true;
  stmt->printPretty(outs(), nullptr, LangOpts);
  cout << endl;
}

const string DefectInfo[2][2] = {
  {
    "error:操作符'/'的右操作数为0",
    "error:操作符'%'的右操作数为0"
  },
  {
    "warning:操作符'/'的右操作数可能为0",
    "warning:操作符'%'的右操作数可能为0"
  }
};

class ZeroVisitor : public RecursiveASTVisitor<ZeroVisitor> {
private:
    vector<Defect> defects;
    const FunctionDecl *funDecl;
public:
  bool VisitBinaryOperator(BinaryOperator *E) {
    if (E->getOpcodeStr() == "/" || E->getOpcodeStr() == "%") {
        string opt = E->getOpcodeStr();
        Expr* ro = E->getRHS()->IgnoreParenCasts()->IgnoreImpCasts();
        Expr::EvalResult rst;
        Expr::ConstExprUsage Usage = Expr::EvaluateForCodeGen;
        if(ro->EvaluateAsConstantExpr(rst, Usage, funDecl->getASTContext())){
          //string begin = ro->getBeginLoc().printToString(funDecl->getASTContext().getSourceManager());
          //cout << begin << endl;
          Defect d;
          auto &[location, info] = d;
          auto &sm = funDecl->getASTContext().getSourceManager();
          APValue::ValueKind vtp = rst.Val.getKind();
          switch(vtp){
            case APValue::Int:{
              int64_t val = rst.Val.getInt().getExtValue();
              //cout <<val<<endl;
              if(val == 0){
                location = ro->getBeginLoc().printToString(sm);
                info = "error:操作符"+opt+"的右操作数是结果为0的常数表达式";
                defects.push_back(d);
              }
            }break;
            case APValue::Float:{
              float val = rst.Val.getFloat().convertToFloat();
              //cout <<val<<endl;
              if(val == 0){
                location = ro->getBeginLoc().printToString(sm);
                info = "error:操作符"+opt+"的右操作数是结果为0的常数表达式";
                defects.push_back(d);
              }
            }break;
            case APValue::FixedPoint:{
              int64_t val = rst.Val.getFixedPoint().getValue().getExtValue();
              //cout <<val<<endl;
              if(val == 0){
                location = ro->getBeginLoc().printToString(sm);
                info = "error:操作符"+opt+"的右操作数是结果为0的常数表达式";
                defects.push_back(d);
              }
            }break;
            default:break;
          }
        }
    }
    return true;
    }

  
  void getFun(const FunctionDecl *funDecl){
    this->funDecl = funDecl;
  }

    const vector<Defect> &getDefects() const { return defects; }

};
}

void ZeroChecker::check() {
    std::vector<ASTFunction *> Funcs = resource->getFunctions();
    for (auto fun : Funcs) {
        // ValueList.clear();
        const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
        getFunDecl(funDecl);
        auto stmt = funDecl->getBody();
        ZeroVisitor visitor;
        visitor.getFun(funDecl);
        visitor.TraverseStmt(stmt);
        //auto dfts = visitor.getDefects();
        //visitFunctionStmts(funDecl->getBody());
        //if(funDecl->getNameAsString() == "testZero1"){
        //cout << "testZero1 start" << endl;
        Analyzer analyzer;
        analyzer.setFunName(funDecl->getNameAsString());
        analyzer.bindZeroChecker(this);
        analyzer.DealFunctionDecl(funDecl);
        //analyzer.printValueList();
        //cout << "testZero1 end" << endl;
        //}
    }
    // defectsClearSamePlace();
}

void ZeroChecker::report(const Expr *expr, int level) {
  Defect d;
  auto &[location, info] = d;
  BinaryOperator *E = (BinaryOperator *)expr;
  Expr* ro = E->getRHS()->IgnoreParenCasts()->IgnoreImpCasts();
  int opt = E->getOpcodeStr() == "/" ? 0 : 1;
  auto &sm = funDecl->getASTContext().getSourceManager();
  location = ro->getBeginLoc().printToString(sm);
  info = DefectInfo[level][opt];
  addDefect(move(d));
}
/*
void ZeroChecker::visitFunctionStmts(Stmt *stmt){
  string cname(stmt->getStmtClassName());
  if(cname == "CompoundStmt"){
    CompoundStmt *cst = (CompoundStmt*)stmt;
    for(auto it = cst->body_begin(); it != cst->body_end(); it++){
      DealStmt(*it);
    }
  }
  //printValueList();
}
*/
/*
void ZeroChecker::DataFlowAnalysis(CFG* cfg){
  for(auto block=cfg->rbegin();block!=cfg->rend();block++){
    for(auto &element : **block){
      if(element.getKind()==CFGStmt::Statement){
        Stmt *stmt = const_cast<Stmt*>(element.castAs<CFGStmt>().getStmt());
        DealStmt(stmt);
      }
    }
  }
  printValueList();
}
*/
/*
void ZeroChecker::DealStmt(Stmt* stmt){
  if(stmt == nullptr)return;
  string cname(stmt->getStmtClassName());
  if(cname == "DeclStmt"){
    DeclStmt *decltmp = (DeclStmt*)stmt;
    if(decltmp->isSingleDecl()){
      DealVarDecl((VarDecl*)(decltmp->getSingleDecl()));
    }
  }
  else if(cname == "BinaryOperator"){
    BinaryOperator *bopt = (BinaryOperator*)stmt;
    DealBinaryOperator(bopt);
  }
  else if(cname == "UnaryOperator"){
    UnaryOperator *uopt = (UnaryOperator*)stmt;
    DealUnaryOperator(uopt);
  }
  else if(cname == "IfStmt"){
    IfStmt *ist = (IfStmt*)stmt;
    DealIfStmt(ist);
  }
  else if(cname == "CompoundStmt"){
    CompoundStmt* cst = (CompoundStmt*)stmt;
    DealCompoundStmt(cst);
  }
  else if(cname == "SwitchStmt"){
    SwitchStmt* sst = (SwitchStmt*)stmt;
    DealSwitchStmt(sst);
  }
  else if(cname == "ForStmt"){
    ForStmt* fst = (ForStmt*)stmt;
    DealForStmt(fst);
  }
  else if(cname == "WhileStmt"){
    WhileStmt* wst = (WhileStmt*)stmt;
    DealWhileStmt(wst);
  }
  else if(cname == "DoStmt"){
    DoStmt* dst = (DoStmt*)stmt;
    DealDoStmt(dst);
  }
}

void ZeroChecker::DealVarDecl(VarDecl* var){
  VarValue new_var;
  new_var.var = var;
  if(var->hasInit()){
    var->evaluateValue();
  }
  else{
    new_var.isDefined = false;
    new_var.PosValue.insert(0);
  }
  if(var->getEvaluatedValue()!=nullptr){
    new_var.isDefined = true;
    switch(var->getEvaluatedValue()->getKind()){
      case APValue::Int:{
              int64_t val = var->getEvaluatedValue()->getInt().getExtValue();
              new_var.var_type = V_INT;
              new_var.PosValue.insert(val);
      }break;
      case APValue::Float:{
              float val = var->getEvaluatedValue()->getFloat().convertToFloat();
              new_var.var_type = V_FLOAT;
              new_var.PosValue.insert(val);
      }break;
      default:{
              new_var.PosValue = (DealRValExpr(var->getInit())).PosValue;
      }break;
    }
  }
  else{
    //new_var.PosValue = DealRValExpr(var->getInit());
  }
  ValueList.push_back(new_var);
}

void ZeroChecker::DealIfStmt(IfStmt *ist){
  VarDecl* new_var = ist->getConditionVariable();
  if(new_var!=nullptr){
    DealVarDecl(new_var);
    return;
  }
  VarValue condres = DealRValExpr(ist->getCond());
  for(auto i:condres.PosValue){
  }
  if(condres.PosValue.find(0)!=condres.PosValue.end()){
    if(condres.PosValue.size()==1){
      Stmt* elst = ist->getElse();
      if(elst!=nullptr){
        DealCompoundStmt(elst);
      }
    }
    else{
      vector<VarValue> stored(ValueList);
      Stmt* thst = ist->getThen();
      if(thst!=nullptr){
        DealCompoundStmt(thst);
      }
      vector<VarValue> tmp(stored);
      stored = ValueList;
      ValueList = tmp;
      Stmt* elst = ist->getElse();
      if(elst!=nullptr){
        DealCompoundStmt(elst);
      }
      bool new_var;
      for(auto i:stored){
        new_var = true;
        for(auto j:ValueList){
          if(j.var->getNameAsString() == i.var->getNameAsString()){
            new_var = false;
            if(!i.isDefined)break;
            for(auto m:i.PosValue){
              j.PosValue.insert(m);
            }
            break;
          }
          if(new_var){
            ValueList.push_back(i);
          }
        }
      }
    }
  }
  else{
    Stmt* thst = ist->getThen();
    if(thst!=nullptr){
      DealCompoundStmt(thst);
    }
  }
}

void ZeroChecker::DealCompoundStmt(Stmt *stmt){
  string cname(stmt->getStmtClassName());
  if(cname == "CompoundStmt"){
    CompoundStmt* cst = (CompoundStmt*)stmt;
    for(auto it = cst->body_begin(); it != cst->body_end(); it++){
      DealStmt(*it);
    }
  }
  else{
    DealStmt(stmt);
  }
}

void ZeroChecker::DealSwitchStmt(SwitchStmt *sst){
  VarValue condvar = DealRValExpr(sst->getCond());
  SwitchCase *case_begin = sst->getSwitchCaseList();
  vector<VarValue> stored;
  bool canVisit = false;
  DefaultStmt *dest=nullptr;
  while(case_begin!=nullptr){
    string case_name(case_begin->getStmtClassName());
    stored = ValueList;
    if(case_name == "CaseStmt"){
      CaseStmt* cast = (CaseStmt*)case_begin;
      canVisit = judgeConsist(condvar, DealRValExpr(cast->getLHS()));
      if(canVisit){
        Stmt* sub = cast->getSubStmt();
        string subname(sub->getStmtClassName());
        while(subname == "CaseStmt"){
          sub = ((CaseStmt*)sub)->getSubStmt();
          subname = sub->getStmtClassName();
        }
        DealCompoundStmt(sub);
      }
    }
    else if(case_name == "DefaultStmt"){
      dest = (DefaultStmt*)case_begin;
    }
    case_begin = case_begin->getNextSwitchCase();
    if(canVisit){
      bool new_var;
      for(auto i:stored){
        new_var = true;
        for(int j=0;j<ValueList.size();j++){
          if(ValueList[j].var->getNameAsString() == i.var->getNameAsString()){
            new_var = false;
            if(!i.isDefined)break;
            for(auto m:i.PosValue){
              ValueList[j].PosValue.insert(m);
            }
            break;
          }
        }
        if(new_var){
          ValueList.push_back(i);
        }
      }
      break;
    }
  }
  if(!canVisit){
    if(dest == nullptr){
      return;
    }
      DealCompoundStmt(dest->getSubStmt());
      bool new_var;
      for(auto i:stored){
        new_var = true;
        for(int j=0;j<ValueList.size();j++){
          if(ValueList[j].var->getNameAsString() == i.var->getNameAsString()){
            new_var = false;
            if(!i.isDefined)break;
            for(auto m:i.PosValue){
              ValueList[j].PosValue.insert(m);
            }
            break;
          }
        }
        if(new_var){
          ValueList.push_back(i);
        }
      }
  }
}

void ZeroChecker::DealForStmt(ForStmt *fst){
  DealStmt(fst->getInit());
  VarValue res;
  if(fst->getCond()!=nullptr){
    res = DealRValExpr(fst->getCond());
  }
  else{
    res.PosValue.insert(1);
  }
  if(res.PosValue.find(0)!=res.PosValue.end()){
    if(res.PosValue.size()==1){
      
    }
    else{
      DealCompoundStmt(fst->getBody());
      DealStmt(fst->getInc());
    }
  }
  else{
    DealCompoundStmt(fst->getBody());
    DealStmt(fst->getInc());
  }
}

void ZeroChecker::DealWhileStmt(WhileStmt *wst){
  VarValue res = DealRValExpr(wst->getCond());
  if(res.PosValue.find(0)!=res.PosValue.end()){
    if(res.PosValue.size()==1){
      
    }
    else{
      DealCompoundStmt(wst->getBody());
    }
  }
  else{
    DealCompoundStmt(wst->getBody());
  }
}

void ZeroChecker::DealDoStmt(DoStmt *dst){
  DealCompoundStmt(dst->getBody());
}

VarValue ZeroChecker::DealRValExpr(Expr* expr){
  string stmt_class = expr->getStmtClassName();
  VarValue PosResult;
  if(stmt_class == "ParenExpr"){
    PosResult = DealRValExpr(((ParenExpr*)expr)->getSubExpr());
  }
  else if(stmt_class == "ConstantExpr"){
    ConstantExpr* cex = (ConstantExpr*)expr;
    switch(cex->getAPValueResult().getKind()){
      case APValue::Int:{
            int64_t val = cex->getAPValueResult().getInt().getExtValue();
            PosResult.var_type = V_INT;
            PosResult.PosValue.insert(val);
      }break;
      case APValue::Float:{
            float val = cex->getAPValueResult().getFloat().convertToFloat();
            PosResult.var_type = V_FLOAT;
            PosResult.PosValue.insert(val);
      }break;
      default:{
      }break;
    }
  }
  else if(stmt_class == "IntegerLiteral"){
    PosResult.var_type = V_INT;
    PosResult.PosValue.insert(((IntegerLiteral*)expr)->getValue().getLimitedValue());
  }
  else if(stmt_class == "FloatingLiteral"){
    PosResult.var_type = V_FLOAT;
    PosResult.PosValue.insert(((FloatingLiteral*)expr)->getValue().convertToFloat());
  }
  else if(stmt_class == "ImplicitCastExpr"){
    string sub_class = (((ImplicitCastExpr*)expr)->getSubExpr())->getStmtClassName();
    if(sub_class == "DeclRefExpr"){
      DeclRefExpr* sub = (DeclRefExpr*)(((ImplicitCastExpr*)expr)->getSubExpr());
      int pos = FindVarInList((VarDecl*)(sub->getDecl()));
      if(pos != -1){
        return ValueList[pos];
      }
      else{
        PosResult.PosValue.insert(0);
      }
    }
    else if(sub_class == "UnaryOperator"){
      PosResult = DealUnaryOperator((UnaryOperator*)(((ImplicitCastExpr*)expr)->getSubExpr()));
    }
    else if(sub_class == "ConditionalOperator"){
      return DealRValExpr(((ImplicitCastExpr*)expr)->getSubExpr());
    }
    else if(sub_class == "ImplicitCastExpr"){
      return DealRValExpr(((ImplicitCastExpr*)expr)->getSubExpr());
    }
  }
  else if(stmt_class == "DeclRefExpr"){
    DeclRefExpr* sub = (DeclRefExpr*)expr;
    int pos = FindVarInList((VarDecl*)(sub->getDecl()));
    if(pos != -1){
      return ValueList[pos];
    }
    else{
      PosResult.PosValue.insert(0);
    }
  }
  else if(stmt_class == "BinaryOperator"){
    PosResult = DealBinaryOperator((BinaryOperator*)expr);
  }
  else if(stmt_class == "UnaryOperator"){
    PosResult = DealUnaryOperator((UnaryOperator*)expr);
  }
  else if(stmt_class == "ConditionalOperator"){
    PosResult = DealConditionalOperator((ConditionalOperator*)expr);
  }
  return PosResult;
}

VarValue ZeroChecker::DealBinaryOperator(BinaryOperator* E){
  VarValue PosResult;
  switch(E->getOpcode()){
    case BO_Assign:{
      string LClass(E->getLHS()->getStmtClassName());
      if(LClass!="DeclRefExpr"){
        return PosResult;
      }
      int pos = FindVarInList((VarDecl*)(((DeclRefExpr*)(E->getLHS()))->getDecl()));
      if(pos != -1){
        ValueList[pos].PosValue = (DealRValExpr(E->getRHS())).PosValue;
        ValueList[pos].isDefined = true;
        PosResult.PosValue.insert(1);
      }
      else{
        PosResult.PosValue.insert(0);
      }
    }break;
    case BO_Add:{
      return DealAddOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
    }break;
    case BO_Sub:{
      return DealSubOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
    }break;
    case BO_Mul:{
      return DealMulOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
    }break;
    case BO_Div:{
      return DealDivOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()), E);
    }break;
    case BO_Rem:{
      return DealModOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()), E);
    }break;
    case BO_LAnd:{
      return DealLogAndOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
    }break;
    case BO_LOr:{
      return DealLogOrOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
    }
    case BO_LT:{
      return DealLTOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
    }break;
    case BO_LE:{
      return DealLEOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
    }break;
    case BO_GT:{
      return DealGTOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
    }break;
    case BO_GE:{
      return DealGEOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
    }break;
    case BO_EQ:{
      return DealEQOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
    }break;
    case BO_NE:{
      return DealNEOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
    }break;
    default:break;
  }
  return PosResult;
}

VarValue ZeroChecker::DealUnaryOperator(UnaryOperator* E){
  VarValue PosResult = DealRValExpr(E->getSubExpr());
  switch(E->getOpcode()){
    case UO_PostInc:{
      PosResult = DealPostInc(PosResult);
    }break;
    case UO_PostDec:{
      PosResult = DealPostDec(PosResult);
    }break;
    case UO_PreInc:{
      PosResult = DealPreInc(PosResult);
    }break;
    case UO_PreDec:{
      PosResult = DealPreDec(PosResult);
    }break;
    case UO_Plus:{
      PosResult = DealPlus(PosResult);
    }break;
    case UO_Minus:{
      PosResult = DealMinus(PosResult);
    }break;
    case UO_Not:{

    }break;
    case UO_LNot:{

    }break;
    default:break;
  }
  return PosResult;
}

VarValue ZeroChecker::DealConditionalOperator(ConditionalOperator* E){
  VarValue cond = DealRValExpr(E->getCond());
  if(cond.PosValue.find(0)!=cond.PosValue.end()){
    if(cond.PosValue.size()==1){
      return DealRValExpr(E->getFalseExpr());
    }
    else{
      VarValue trueres = DealRValExpr(E->getTrueExpr());
      VarValue falseres = DealRValExpr(E->getFalseExpr());
      for(auto i:falseres.PosValue){
        trueres.PosValue.insert(i);
      }
      return trueres;
    }
  }
  else{
    return DealRValExpr(E->getTrueExpr());
  }
}*/
//visit some specific children class by Stmt*
/*
  Stmt *st = block->getTerminatorStmt();
  if(st != nullptr){
    string stp(st->getStmtClassName());
    if(stp == "ForStmt"){
      ((ForStmt*)st)->getBody()->dump();
    }
  }
  */

  /*for (auto *block: *cfg){
    for (auto &element : *block){
      if(element.getKind()==CFGStmt::Statement){
        Stmt *stmt = const_cast<Stmt*>(element.castAs<CFGStmt>().getStmt());
        stmt->dump();
        cout << stmt->getStmtClassName() << endl;
      }
    }
  }*/
