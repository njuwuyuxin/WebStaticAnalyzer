#include "checkers/LoopChecker.h"
#include "clang/Analysis/Analyses/LiveVariables.h"
#include "clang/Analysis/AnalysisDeclContext.h"

namespace {

    static inline void printStmt(const Stmt *stmt, const SourceManager &sm,string info = "")
    {   //Originate for ZeroChecker
        string begin = stmt->getBeginLoc().printToString(sm);
        cout << begin << endl;
        LangOptions LangOpts;
        LangOpts.CPlusPlus = true;
        stmt->printPretty(outs(), nullptr, LangOpts,5);
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
        const ASTContext &CTX;
        std::unique_ptr<CFG> &CFG;
        const FunctionDecl* funDecl;

        //Source Manager
        const SourceManager &sm;

        bool check_CFG()
        {
            //TODO:Data FLow Analyze
            clang::AnalysisDeclContextManager AM(const_cast<clang::ASTContext&>(CTX));
            auto AnalysisDeclContext = AM.getContext(funDecl);
            auto result = LiveVariables::computeLiveness(*AnalysisDeclContext,false);

            //TODO get Loop Control Variables in result



            //Avalible Expression Analyze ?
            return true;
        }


        bool check_Expresion(Stmt* stmt)
        {
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
            
            string Defect_Description;

            if(conditionExpr!=nullptr)
            {
                printStmt(conditionExpr, sm);
                llvm::APSInt Result(0);
                //檢查表達式中是否值恆為常數
                if (conditionExpr->isIntegerConstantExpr(Result, CTX))//Check wheather the Expression can be fold into a integer
                {
                    if(Result!=0)    //if true and the Expression result is always not Zero, then we find an Infinety Loop
                    {   
                        Defect_Description = "循環條件恆爲" + Result.toString(10);
                        stmts.push_back({stmt,//defect Statement
                                        Defect_Description});//Defect Info
                    }
                }
                else //conditionExpr is a Varient
                {
                    printStmt(conditionExpr,sm,"Condition cannot fold into Integer!");

                }
            }
            else
            {   //No Condition Expression in the loop
                Defect_Description = "循環缺乏跳出條件";
                printStmt(stmt, sm,Defect_Description);
                stmts.push_back({stmt,//defect Statement
                               Defect_Description});//Defect Info
            }
            return true;
        }


    public:
        LoopVisitor(const ASTContext &ctx, std::unique_ptr<clang::CFG> &cfg,  const SourceManager &sm,const FunctionDecl* funDecl)
                    :CTX(ctx),CFG(cfg),sm(sm),funDecl(funDecl){}

        const vector<DefectInfo> &getStmts() const { return stmts; }

        bool VisitWhileStmt(WhileStmt* stmt)    //when find while program point enter this function
        {
            bool ExprResult = check_Expresion(stmt);
            bool CFGResult = check_CFG();
            return ExprResult && CFGResult;
        }

        bool VisitForStmt(ForStmt* stmt)    //when find for program point enter this function
        {
            bool ExprResult = check_Expresion(stmt);
            bool CFGResult = check_CFG();
            return ExprResult && CFGResult;
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

        auto &CurrentFuncCFG = manager->getCFG(func);

        LoopVisitor vistor(ASTContext,CurrentFuncCFG,sm,funDecl);    //visit AST
        vistor.TraverseStmt(stmt);
        auto DefectList = vistor.getStmts();


        for (auto &&DefectStmt : DefectList) {
          Defect d;
          d.location = DefectStmt.Statement->getBeginLoc().printToString(sm);
          d.info = "存在可能的死循環 ("+ DefectStmt.info + ")";
          defects.push_back(d);
        }
    }



    return defects;
}

