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

std::vector<Defect> LoopAnalyze::check()
{
    getEntryFunc();
    //TODO

    std::vector<Defect> result = new std::vector<Defect>; 


    return result;
}