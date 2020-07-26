#include "checkers/CharArrayBound.h"

#include <clang/ASTMatchers/ASTMatchFinder.h>
#include <clang/ASTMatchers/ASTMatchers.h>

#include "framework/Analyzer.h"

using namespace clang::ast_matchers;

namespace {
/*
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
*/
class CharArrayCheck : public MatchFinder::MatchCallback {
  MatchFinder Matcher;
  BasicChecker *checker;

  void run(const MatchFinder::MatchResult &Result) override {
    const auto *ArraySubscriptE =
        Result.Nodes.getNodeAs<ArraySubscriptExpr>("expr");
    const auto *VarD = Result.Nodes.getNodeAs<VarDecl>("base");
    const auto *IndexE = Result.Nodes.getNodeAs<Expr>("index");

    auto val = VarD->evaluateValue();
    int len = 0;
    if (val && val->isArray()) {
      len = val->getArrayInitializedElts();
    }
    APSInt Index;
    if (IndexE->isIntegerConstantExpr(Index, *Result.Context)) {
      if (Index > len) {
        checker->report(ArraySubscriptE, len == 0);
      }
    }
  }

public:
  CharArrayCheck(BasicChecker *checker) : checker(checker) {
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
};

const string DefectInfo[2] = {
    R"(error:通过下标访问字符数组'\0'之后的内容)",
    R"(warning:可能通过下标访问字符数组'\0'之后的内容)"};

} // namespace

void CharArrayBound::check() {
  vector<ASTFile *> ASTs = resource->getASTFiles();
  CharArrayCheck checker(this);
  for (auto &&AST : ASTs) {
    auto &ctx = manager->getASTUnit(AST)->getASTContext();
    sm = &ctx.getSourceManager();
    checker.check(ctx);
  }

  Analyzer analyzer;
  analyzer.bindCharArrayChecker(this);
  std::vector<ASTFunction *> functions = resource->getFunctions();
  for (auto fun : functions) {
    const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
    sm = &funDecl->getASTContext().getSourceManager();
    analyzer.DealStmt(funDecl->getBody());
  }
}

void CharArrayBound::report(const Expr *expr, int level) {
  addDefect(make_tuple(expr->getExprLoc().printToString(*sm), DefectInfo[level]));
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
