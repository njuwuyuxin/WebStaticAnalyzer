#include "checkers/LoopChecker.h"
// #include "clang/Analysis/Analyses/LiveVariables.h"
// #include "clang/Analysis/AnalysisDeclContext.h"

// #include <llvm/ADT/SCCIterator.h>

#define Debug

#ifdef Debug
static inline void debug(string target)
{
    std::cout << target << endl;
}
#endif

namespace
{
    static inline void printStmt(const Stmt *stmt, const SourceManager *sm = nullptr, string info = "", int indent = 0)
    { //Originate for ZeroChecker
#ifdef Debug
        if (!stmt)
        {
            debug("!!!                 The Given Satement is Empty                 !!!");
            return;
        }

        if (sm)
        {
            string begin = stmt->getBeginLoc().printToString(*sm);
            cout << begin << endl;
        }

        for (int i = 0; i < indent; ++i)
            cout << "\t";

        LangOptions LangOpts;
        LangOpts.CPlusPlus = true;
        stmt->printPretty(outs(), nullptr, LangOpts, 5);
        // stmt->dumpColor();

        if (info != "")
        {
            cout << " " << info;
        }
        cout << endl;
#endif
    }

    class LoopVisitor : public RecursiveASTVisitor<LoopVisitor>
    {
    private:
        struct DefectInfo
        {
            Stmt *Statement;
            std::string info;
        };

        vector<DefectInfo> stmts;
        const ASTContext &CTX;
        const FunctionDecl *funDecl;

        //Source Manager
        const SourceManager &sm;
        // clang::LiveVariables *LivenessResult;

        //Recursively Find Loop Exit(break or return ) on Statement-Level
        bool FindLoopExit(Stmt *target)
        {
            bool ret = false;
            if(target)
            {
                if (BreakStmt::classof(target) || ReturnStmt::classof(target))
                    return true;

                //Check target's childeren
                for (auto it : target->children())
                {
                    ret = FindLoopExit(it);
                }
            }
            return ret;
        }

        bool check_Expresion(Stmt *stmt)
        {
            Expr *conditionExpr = nullptr;
            VarDecl *CondDecl = nullptr;
            // bool CondLiveness = false;

            printStmt(stmt);
            debug("Current Stmt Child");
            for (auto i : stmt->children()) //dump all child
            {
                i->dumpColor();
            }

            if (ForStmt::classof(stmt))
            {
                conditionExpr = reinterpret_cast<ForStmt *>(stmt)->getCond();
                // CondDecl = reinterpret_cast<ForStmt *>(stmt)->getConditionVariable();
            }
            else if (WhileStmt::classof(stmt))
            {
                conditionExpr = reinterpret_cast<WhileStmt *>(stmt)->getCond();
                // CondDecl = reinterpret_cast<WhileStmt *>(stmt)->getConditionVariable();
            }
            else
            {
                debug("Something Wrong with check Expression!");
                return false;
            }

            // if (CondDecl)
            // {
            //     CondLiveness = LivenessResult->isLive(stmt, CondDecl);
            //     printStmt(stmt,sm,"Successfully get Condition Declaration!");
            // }
            // else
            // {
            //     printStmt(stmt, sm, "Cannot get Condition Declaration!!");
            // }

            string Defect_Description("None");

            auto exit = FindLoopExit(stmt);
            #ifdef Debug
                cout<<"Loop Exit : "<<exit<<endl;
            #endif

            //Data Flow Analysis require for a Deeper check
            auto DFA_require = false;
            if (conditionExpr != nullptr)
            {
                // printStmt(conditionExpr, sm);
                llvm::APSInt Result;
                //檢查表達式中是否值恆為常數
                if (conditionExpr->isIntegerConstantExpr(Result, CTX)) //Check wheather the Expression can be fold into a integer
                {
                    if (Result != 0) //if true and the Expression result is always not Zero, then we find an Infinety Loop
                    {
                        if (!exit)
                        {
                            Defect_Description = "循環條件恆爲" + Result.toString(10) + ", 且缺乏break/return語句";
                            printStmt(conditionExpr, nullptr, Defect_Description);
                        }
                        else
                        {
                            DFA_require = true;
                        }
                    }
                }
                else //conditionExpr is a Varient
                {
                    // printStmt(conditionExpr, &sm, "Condition cannot fold into Integer!");
                    // cout<<"Condition Liveness : "<<CondLiveness<<endl;
                        DFA_require = true;
                }
            }
            else if (!exit)
            { //No Condition Expression in the loop and has no break or return statement
                Defect_Description = "循環缺乏跳出條件 (設定循環變量範圍語句 或 break、return語句)";
                printStmt(stmt, &sm, Defect_Description);
            }
            else
            {
                DFA_require = true;
            }

            #ifdef Debug
            if(DFA_require)
                debug("Need Further Data Flow Analysis");
            debug("");
            debug("");
            #endif

            if (Defect_Description != "None")
                stmts.push_back({stmt,                 //defect Statement
                                 Defect_Description}); //Defect Info

            return DFA_require;
        }

        //LayerAmount : 嵌套層數
        bool check_CFG(Stmt *stmt, int LayerAmount)
        {
            // if (LayerAmount)
            //     printStmt(stmt, nullptr, "", LayerAmount);
            // else
            //     printStmt(stmt, &sm);

            CFG::BuildOptions BO;
            // BO.AddLoopExit = true;
            ASTContext &temp = const_cast<ASTContext &>(CTX);
            auto Loop_CFG = CFG::buildCFG(funDecl, stmt, &temp, BO);
            for (auto i : *Loop_CFG)
            {
                // i->dump();
                // i->getLoopTarget()->dumpColor();

                // if(i != &Loop_CFG->getEntry() && i!=&Loop_CFG->getExit())
                // {
                //     for (int i = 0; i < LayerAmount; ++i)
                //         cout << "\t";
                //     cout << "B";
                //     cout << i->getBlockID() << endl;
                // }

                // //獲得嵌套循環體
                // if (auto target = i->getLoopTarget())
                // {
                //     // printStmt(target);
                //     if(target != stmt)
                //         check_CFG(const_cast<Stmt *>(target),LayerAmount+1);
                // }

                // for (auto element : *i) //Show Element in CFGBlock
                // {
                //     if (element.getKind() == CFGElement::Statement)
                //     {
                //         CFGStmt *tmp = (CFGStmt *)&element;
                //         printStmt(tmp->getStmt());
                //     }
                // }

                // cout << "----------------------" << endl;
            }

            // LangOptions LangOpts;
            // LangOpts.CPlusPlus = true;
            // Loop_CFG->dump(LangOpts, true);
            return true;
        }

    public:
        // LoopVisitor(const ASTContext &ctx, const SourceManager &sm, clang::LiveVariables *liveness)
        //     : CTX(ctx), sm(sm), LivenessResult(liveness) {}
        LoopVisitor(const ASTContext &ctx, const SourceManager &sm, const FunctionDecl *funDecl)
            : CTX(ctx), sm(sm), funDecl(funDecl) {}

        const vector<DefectInfo> &getStmts() const { return stmts; }

        bool VisitWhileStmt(WhileStmt *stmt) //when find while program point enter this function
        {
            bool DFA_require = check_Expresion(stmt);
            // bool CFGResult = check_CFG(stmt, 0);

            if (DFA_require)
            {
                //TODO : Data Flow Analysis
            }

            // auto CondDel = stmt->getConditionVariableDeclStmt();
            // printStmt(CondDel,sm);
            // cout << LivenessResult->isLive(stmt,CondDel) <<endl<<endl;

            return true;
        }

        bool VisitForStmt(ForStmt *stmt) //when find for program point enter this function
        {
            bool DFA_require = check_Expresion(stmt);

            // printStmt(LoopInc, &sm);

            if (!stmt->getInc())
            {
                stmts.push_back({stmt, "(warning)缺少控制變量變化條件"});
            }

            if (DFA_require)
            {
                //TODO : Data Flow Analysis
            }
            // bool CFGResult = check_CFG(stmt, 0);
            // return ExprResult && CFGResult;
            // auto CondDel = stmt->getConditionVariableDeclStmt();
            // printStmt(CondDel,sm);
            // cout << LivenessResult->isLive(stmt,CondDel) <<endl<<endl;
            return true;
        }
    };

} //namespace

// bool LoopChecker::check_CFG(std::unique_ptr<clang::CFG> &cfg, const ASTContext &ctx, const FunctionDecl *funDecl)
// {
//     if (!cfg)
//         return false;
//
//     // LangOptions LangOpts;
//     // LangOpts.CPlusPlus = true;
//     // CFG->print(outs(),LangOpts,5 );
//
//     //TODO: Need to check  whether CFGLoopExit Block exsit in CFG
//     // for (auto it : cfg)
//     // {
//     //     it;
//     // }
//
//     //TODO:Data FLow Analyze
//
//     //TODO get Loop Control Variables in result
//     // result->dumpStmtLiveness(sm);
//     // result->isLive()
//
//     //Avalible Expression Analyze ?
//     return true;
// }

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

    for (auto func : functions)
    {
        const FunctionDecl *funDecl = manager->getFunctionDecl(func); //get function declaration
        auto stmts = funDecl->getBody();                              //get body of the function through getbody() -> Clang::Stmt type
        const ASTContext &ASTContext = funDecl->getASTContext();
        auto &sm = ASTContext.getSourceManager();

        // auto &CurrentFuncCFG = manager->getCFG(func);

        //Get Liveness Result
        // clang::AnalysisDeclContextManager AM(const_cast<clang::ASTContext &>(ASTContext));
        // auto AnalysisDeclContext = AM.getContext(funDecl);
        // auto result = LiveVariables::computeLiveness(*AnalysisDeclContext, false);

        // result->dumpStmtLiveness(sm);

        //Traverse AST
        // LoopVisitor vistor(ASTContext, sm, result);
        LoopVisitor vistor(ASTContext, sm, funDecl);
        vistor.TraverseStmt(stmts);

        // check_CFG(CurrentFuncCFG,ASTContext,funDecl);

        auto DefectList = vistor.getStmts();

        for (auto &&DefectStmt : DefectList)
        {
            Defect d;
            d.location = DefectStmt.Statement->getBeginLoc().printToString(sm);
            d.info = "存在可能的死循環 (" + DefectStmt.info + ")";
            defects.push_back(d);
        }
    }

    return defects;
}
