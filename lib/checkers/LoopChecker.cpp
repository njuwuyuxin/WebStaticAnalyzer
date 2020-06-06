#include "checkers/LoopChecker.h"

namespace {

    class LoopVisitor : public RecursiveASTVisitor<LoopVisitor>
    {
    private:
        struct DefectInfo
        {
            Stmt* Statement;
            std::string info; 
        };

        vector< DefectInfo > stmts;
        const ASTContext &ctx;
        std::unique_ptr<CFG> &cfg;

    public:
        LoopVisitor(const ASTContext &ctx, std::unique_ptr<CFG> &cfg):ctx(ctx),cfg(cfg){}

        const vector<DefectInfo> &getStmts() const { return stmts; }

        bool VisitWhileStmt(WhileStmt* stmt)    //when find while program point enter this function
        {
            //do Data Flow Analysis using CFG?

            //Current While Condition
            Expr* conditionExpr = stmt->getCond();
            if(conditionExpr!=nullptr)
            {
                llvm::APSInt Result;
                if (conditionExpr->isIntegerConstantExpr(Result, ctx))//Check wheather the Expression can be fold into a integer
                {
                    if(Result!=0)    //if true and the Expression result is always bigger than one, then we find an Infinety Loop
                    {   
                        stmts.push_back({stmt,//defect Statement
                                        "循環條件恆爲" + Result.toString(10)});//Defect Info
                    }
                }
                else //conditionExpr is a Varient
                {
                    //TODO:need to get Condition String
                    //Source manager to get source code loacation
                    std::cout<<"Condition cannot fold into Integer!\n";

                }
                return true;

                // bool BooleanValue;
                // //檢查當前while檢查表達式中是否值恆為常數
                // if(conditionExpr->EvaluateAsBooleanCondition(BooleanValue,ctx))//Expresion can be fold and convert to a boolean condition
                // {   if(BooleanValue) //if the Expretion ans is always true
                //     {    
                //         //TODO get Condition String! Check others Code for more information
                //         stmts.push_back(conditionExpr);
                //         return true;
                //     }
                //     return true;
                // }

                // //check whether current condition can be fold and convert to a integer
                // clang::Expr::EvalResult IntegerValue;
                // if(conditionExpr->EvaluateAsInt(IntegerValue,ctx))
                // {

                //     return true;
                // }
                // return ans;
            }
            return false;
        }

        bool VisitForStmt(ForStmt* stmt)    //when find for program point enter this function
        {
            Expr* conditionExpr = stmt->getCond();
            if(conditionExpr!=nullptr)
            {
                llvm::APSInt Result;
                if (conditionExpr->isIntegerConstantExpr(Result, ctx))//Check wheather the Expression can be fold into a integer
                {
                    if(Result!=0)    //if true and the Expression result is always bigger than one, then we find an Infinety Loop
                    {   
                        stmts.push_back({conditionExpr,//defect Statement
                                        "循環條件恆爲" + Result.toString(10)});//Defect Info
                    }
                }
                else //conditionExpr is a Varient
                {
                    //TODO:need to get Condition String
                    //Source manager to get source code loacation
                    std::cout<<"Condition cannot fold into Integer!\n";

                }
                return true;
            }
            else
            {
                stmts.push_back({stmt,//defect Statement
                               "循環缺乏跳出條件"});//Defect Info
                return true;
            }
            
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
        const FunctionDecl *funDecl = manager->getFunctionDecl(func);   //get function declaration
        auto stmt = funDecl->getBody();     //get body of the function through getbody() -> Clang::Stmt type
        const ASTContext &ASTContext = funDecl->getASTContext();

        std::unique_ptr<CFG> &CurrentFuncCFG = manager->getCFG(func);

        LoopVisitor vistor(ASTContext,CurrentFuncCFG);    //visit AST
        vistor.TraverseStmt(stmt);  
        auto DefectList = vistor.getStmts();
        auto &sm = ASTContext.getSourceManager();


        for (auto &&DefectStmt : DefectList) {
          Defect d;
          d.location = DefectStmt.Statement->getBeginLoc().printToString(sm);
          d.info = "存在可能的死循環, " + DefectStmt.info;
          defects.push_back(d);
        }
    }



    return defects;
}

