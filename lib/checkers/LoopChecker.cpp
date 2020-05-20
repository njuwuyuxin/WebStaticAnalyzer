#include "checkers/LoopChecker.h"

namespace {
    
    class LoopVisitor : public RecursiveASTVisitor<LoopVisitor>
    {
    private:
        vector<Stmt *> stmts;
        const ASTContext &ctx;


    public:
        LoopVisitor(const ASTContext &ctx):ctx(ctx){}

        const vector<Stmt *> &getStmts() const { return stmts; }

        bool VisitWhileStmt(WhileStmt* stmt)    //when find while program point enter this function
        {
            //檢查當前while檢查表達式中是否值恆為常數
            Expr* conditionExpr = stmt->getCond();
            if(conditionExpr!=nullptr)
            {
                bool ans;
                if(conditionExpr->EvaluateAsBooleanCondition(ans,ctx))//Expresion can be fold and convert to a boolean condition
                {   if(ans) //if the Expretion ans is always true
                        stmts.push_back(conditionExpr);
                }
            }
        }

        bool VisitForStmt(ForStmt* stmt)    //when fine for program point enter this function
        {

        }
    };
    
    }//namespace

void LoopChecker::getEntryFunc()
{
    std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
    for (auto fun : topLevelFuncs)
    {
        const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
        if (funDecl->isMain())
        {
            entryFunc = fun;
            return;
        }
    }
    entryFunc = nullptr;
    return;
}

std::vector<Defect> LoopChecker::check()
{
    std::vector<ASTFunction *> functions = resource->getFunctions();
    std::vector<Defect> defects; 
    
    for(auto func:functions)
    {
        const FunctionDecl *funDecl = manager->getFunctionDecl(func);
        auto stmt = funDecl->getBody();
        const ASTContext &ctx = funDecl->getASTContext();

        LoopVisitor vistor(ctx);
        vistor.TraverseStmt(stmt);
        auto stmts = vistor.getStmts();
        auto &sm = ctx.getSourceManager();
        for (auto &&s : stmts) {
          Defect d;
          d.location = s->getBeginLoc().printToString(sm);
          d.info = "存在可能的死循環";
          defects.push_back(d);
        }
    }



    return defects;
}

