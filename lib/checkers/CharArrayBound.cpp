#include "checkers/CharArrayBound.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/Expr.h"
#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

using namespace clang::ast_matchers;

namespace {

class CharArrayVisitor : public RecursiveASTVisitor<CharArrayVisitor> {
public:
  bool VisitArraySubscriptExpr(ArraySubscriptExpr *E) {
    if (E->getType().getAsString() == "char") {
      const VarDecl *vd = (VarDecl *)E->getBase()->getReferencedDeclOfCallee();
      int len = vd->evaluateValue()->getArrayInitializedElts();
      auto idx = E->getIdx();
      Expr::EvalResult i;
      if (!idx->EvaluateAsInt(i, vd->getASTContext()) || i.Val.getInt() > len) {
        stmts.push_back(E);
      }
    }
    return true;
  }

  const vector<Stmt *> &getStmts() const { return stmts; }

private:
  vector<Stmt *> stmts;
};

class AssignHandler : public MatchFinder::MatchCallback {
  vector<int> values;

public:
  void run(const MatchFinder::MatchResult &Result) override {
    const auto *ValueE = Result.Nodes.getNodeAs<Expr>("value");
    APSInt Value;
    if (ValueE->isIntegerConstantExpr(Value, *Result.Context)) {
      values.push_back(Value.getExtValue());
    }
  }

  const vector<int> &getValues() const { return values; }
};

class CharArrayCheck : public MatchFinder::MatchCallback {
  MatchFinder Matcher;
  vector<const Expr *> exprs;

  void run(const MatchFinder::MatchResult &Result) override {
    const auto *ArraySubscriptE =
        Result.Nodes.getNodeAs<ArraySubscriptExpr>("expr");
    const auto *VarD = Result.Nodes.getNodeAs<VarDecl>("base");
    const auto *IndexE = Result.Nodes.getNodeAs<Expr>("index");

    int len = VarD->evaluateValue()->getArrayInitializedElts();
    APSInt Index;
    if (IndexE->isIntegerConstantExpr(Index, *Result.Context)) {
      if (Index > len) {
        exprs.push_back(ArraySubscriptE);
      }
    } else if (DeclRefExpr::classof(IndexE)) {
      auto indexVar = IndexE->getReferencedDeclOfCallee();
      MatchFinder m;
      AssignHandler h;
      m.addMatcher(
          binaryOperator(
              isAssignmentOperator(),
              hasLHS(ignoringParenCasts(declRefExpr(to(equalsNode(indexVar))))),
              hasRHS(ignoringParenCasts(expr().bind("value")))),
          &h);
      m.addMatcher(
          forStmt(hasCondition(binaryOperator(
              hasLHS(ignoringParenCasts(declRefExpr(to(equalsNode(indexVar))))),
              hasRHS(ignoringParenCasts(expr().bind("value")))))),
          &h);
      m.matchAST(*Result.Context);
      auto values = h.getValues();
      for (auto &&val : values) {
        if (val > len) {
          exprs.push_back(ArraySubscriptE);
          break;
        }
      }
    }
  }

public:
  CharArrayCheck() {
    Matcher.addMatcher(
        arraySubscriptExpr(hasBase(ignoringParenCasts(
                               declRefExpr(to(varDecl().bind("base"))))),
                           hasIndex(ignoringParenCasts(expr().bind("index"))),
                           hasType(asString("char")),
                           unless(hasAncestor(isImplicit())))
            .bind("expr"),
        this);
  }

  void check(ASTContext &Context) { Matcher.matchAST(Context); }

  const vector<const Expr *> &getExprs() const { return exprs; }
};

} // namespace

vector<Defect> CharArrayBound::check() {
  vector<ASTFile *> ASTs = resource->getASTFiles();
  vector<Defect> defects;
  for (auto &&AST : ASTs) {
    auto &ctx = manager->getASTUnit(AST)->getASTContext();
    CharArrayCheck checker;
    checker.check(ctx);
    auto exprs = checker.getExprs();
    auto &sm = ctx.getSourceManager();
    for (auto &&e : exprs) {
      Defect d;
      d.location = e->getExprLoc().printToString(sm);
      d.info = R"(通过下标访问字符数组'\0'之后的内容)";
      defects.push_back(d);
    }
  }

  std::vector<ASTFunction *> functions = resource->getFunctions();
  for (auto fun : functions) {
    const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
  }
  return defects;
}

void CharArrayBound::getEntryFunc() {
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
