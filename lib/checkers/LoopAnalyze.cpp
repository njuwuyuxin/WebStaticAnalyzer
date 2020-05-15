#include "checkers/LoopAnalyze.h"

void LoopAnlyze::getEntryFunc()
{
    std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
    for (auto fun : topLevelFuncs)
    {
        const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
        if (funDecl->getQualifiedNameAsString() == "main")
        {
            entryFunc = fun;
            return;
        }
    }
    entryFunc = nullptr;
    return;
}

void LoopAnalyze::check()
{
    getEntryFunc();
    //TODO
}