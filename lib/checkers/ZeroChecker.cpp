#include "checkers/ZeroChecker.h"

namespace {

static inline void printStmt(const Stmt* stmt, const SourceManager &sm){
  string begin = stmt->getBeginLoc().printToString(sm);
  cout << begin << endl;
  LangOptions LangOpts;
  LangOpts.CPlusPlus = true;
  stmt->printPretty(outs(), nullptr, LangOpts);
  cout << endl;
}

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
          auto &sm = funDecl->getASTContext().getSourceManager();
          APValue::ValueKind vtp = rst.Val.getKind();
          switch(vtp){
            case APValue::Int:{
              int64_t val = rst.Val.getInt().getExtValue();
              //cout <<val<<endl;
              if(val == 0){
                d.location = ro->getBeginLoc().printToString(sm);
                d.info = "操作符"+opt+"的右操作数是结果为0的常数表达式";
                defects.push_back(d);
              }
            }break;
            case APValue::Float:{
              float val = rst.Val.getFloat().convertToFloat();
              //cout <<val<<endl;
              if(val == 0){
                d.location = ro->getBeginLoc().printToString(sm);
                d.info = "操作符"+opt+"的右操作数是结果为0的常数表达式";
                defects.push_back(d);
              }
            }break;
            case APValue::FixedPoint:{
              int64_t val = rst.Val.getFixedPoint().getValue().getExtValue();
              //cout <<val<<endl;
              if(val == 0){
                d.location = ro->getBeginLoc().printToString(sm);
                d.info = "操作符"+opt+"的右操作数是结果为0的常数表达式";
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

vector<Defect> ZeroChecker::check() {
    std::vector<ASTFunction *> Funcs = resource->getFunctions();
    vector<Defect> defects;
    for (auto fun : Funcs) {
        const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
        auto stmt = funDecl->getBody();
        ZeroVisitor visitor;
        visitor.getFun(funDecl);
        visitor.TraverseStmt(stmt);
        auto dfts = visitor.getDefects();
        defects.insert(defects.end(),dfts.begin(),dfts.end());
        if(funDecl->getQualifiedNameAsString()=="testZero2"){
          std::unique_ptr<CFG>& cfg = manager->getCFG(fun);
          DataFlowAnalysis(cfg.get());
      }
    }


    return defects;
}

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

void ZeroChecker::DealStmt(Stmt* stmt){
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
}

void ZeroChecker::DealVarDecl(VarDecl* var){
  VarValue new_var;
  new_var.var = var;
  if(var->hasInit()){
    var->evaluateValue();
  }
  else{
    new_var.PosValue.insert(0);
  }
  if(var->getEvaluatedValue()!=nullptr){
    switch(var->getEvaluatedValue()->getKind()){
      case APValue::Int:{
              int64_t val = var->getEvaluatedValue()->getInt().getExtValue();
              new_var.PosValue.insert(val);
      }break;
      case APValue::Float:{
              float val = var->getEvaluatedValue()->getFloat().convertToFloat();
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

VarValue ZeroChecker::DealRValExpr(Expr* expr){
  string stmt_class = expr->getStmtClassName();
  VarValue PosResult;
  if(stmt_class == "ParenExpr"){
    PosResult = DealRValExpr(((ParenExpr*)expr)->getSubExpr());
  }
  else if(stmt_class == "IntegerLiteral"){
    PosResult.PosValue.insert(((IntegerLiteral*)expr)->getValue().getLimitedValue());
  }
  else if(stmt_class == "FloatingLiteral"){
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
  return PosResult;
}

VarValue ZeroChecker::DealBinaryOperator(BinaryOperator* E){
  string opt = E->getOpcodeStr();
  VarValue PosResult;
  if(opt == "="){
    string LClass(E->getLHS()->getStmtClassName());
    if(LClass!="DeclRefExpr"){
      return PosResult;
    }
    int pos = FindVarInList((VarDecl*)(((DeclRefExpr*)(E->getLHS()))->getDecl()));
    if(pos != -1){
      ValueList[pos].PosValue = (DealRValExpr(E->getRHS())).PosValue;
      PosResult.PosValue.insert(1);
    }
    else{
      PosResult.PosValue.insert(0);
    }
  }
  else if(opt == "+"){
    return DealAddOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  }
  else if(opt == "-"){
    return DealSubOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  }
  else if(opt == "*"){
    return DealMulOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  }
  else if(opt == "/"){
    return DealDivOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
  }
  else if(opt == "%"){
    return DealModOp(DealRValExpr(E->getLHS()), DealRValExpr(E->getRHS()));
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
