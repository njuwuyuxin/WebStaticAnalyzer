#include "checkers/LoopChecker.h"

namespace {

    static inline void printStmt(const Stmt *stmt, const SourceManager &sm,string info = "")
    {   //Originate for ZeroChecker
        string begin = stmt->getBeginLoc().printToString(sm);
        cout << begin << endl;
        LangOptions LangOpts;
        LangOpts.CPlusPlus = true;
        stmt->printPretty(outs(), nullptr, LangOpts);
        cout<<" "<<info;
        cout << endl;
    }

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
        const SourceManager &sm;

        bool check_Expresion(Stmt* stmt)
        {

            //do Data Flow Analysis using CFG?

            Expr* conditionExpr = nullptr;
            
            if(ForStmt::classof(stmt))
            {
                conditionExpr = reinterpret_cast<ForStmt*>(stmt)->getCond();
            }
            else if(WhileStmt::classof(stmt))
            {
                conditionExpr = reinterpret_cast<WhileStmt*>(stmt)->getCond();
            }
            else 
            {
                std::cout<<"Something Wrong with check Expression!"<<endl;
                return false;
            }
            

            if(conditionExpr!=nullptr)
            {
                printStmt(conditionExpr, sm);
                llvm::APSInt Result;
                //檢查表達式中是否值恆為常數
                if (conditionExpr->isIntegerConstantExpr(Result, ctx))//Check wheather the Expression can be fold into a integer
                {
                    if(Result!=0)    //if true and the Expression result is always not Zero, then we find an Infinety Loop
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

            }
            else
            {   //No Condition Expression in the loop
                
                stmts.push_back({stmt,//defect Statement
                               "循環缺乏跳出條件"});//Defect Info
            }
            return true;
        }


    public:
        LoopVisitor(const ASTContext &ctx, std::unique_ptr<CFG> &cfg,  const SourceManager &sm)
                    :ctx(ctx),cfg(cfg),sm(sm){}

        const vector<DefectInfo> &getStmts() const { return stmts; }

        bool VisitWhileStmt(WhileStmt* stmt)    //when find while program point enter this function
        {
            return check_Expresion(stmt);
        }

        bool VisitForStmt(ForStmt* stmt)    //when find for program point enter this function
        {
            // printStmt(stmt,sm);
            // Expr* conditionExpr = stmt->getCond();
            // if(conditionExpr!=nullptr)
            // {
            //     llvm::APSInt Result;
            //     if (conditionExpr->isIntegerConstantExpr(Result, ctx))//Check wheather the Expression can be fold into a integer
            //     {
            //         if(Result!=0)    //if true and the Expression result is always bigger than one, then we find an Infinety Loop
            //         {   
            //             stmts.push_back({conditionExpr,//defect Statement
            //                             "循環條件恆爲" + Result.toString(10)});//Defect Info
            //         }
            //     }
            //     else //conditionExpr is a Varient
            //     {
            //         //TODO:need to get Condition String
            //         //Source manager to get source code loacation
            //         std::cout<<"Condition cannot fold into Integer!\n";
            //     }
            //     return true;
            // }
            // else
            // {
            //     stmts.push_back({stmt,//defect Statement
            //                    "循環缺乏跳出條件"});//Defect Info
            //     return true;
            // }
            return check_Expresion(stmt);
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
        auto &sm = ASTContext.getSourceManager();

        std::unique_ptr<CFG> &CurrentFuncCFG = manager->getCFG(func);

        LoopVisitor vistor(ASTContext,CurrentFuncCFG,sm);    //visit AST
        vistor.TraverseStmt(stmt);  
        auto DefectList = vistor.getStmts();


        for (auto &&DefectStmt : DefectList) {
          Defect d;
          d.location = DefectStmt.Statement->getBeginLoc().printToString(sm);
          d.info = "存在可能的死循環, " + DefectStmt.info;
          defects.push_back(d);
        }
    }



    return defects;
}

