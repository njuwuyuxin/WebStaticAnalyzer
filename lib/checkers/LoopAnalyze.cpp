#include "checkers/LoopAnalyze.h"

void LoopAnalyze::getEntryFunc()
{
    std::vector<ASTFunction *> topLevelFuncs = call_graph->getTopLevelFunctions();
    for (auto fun : topLevelFuncs)
    {
        const FunctionDecl *funDecl = manager->getFunctionDecl(fun);
        if (funDecl->getQualifiedNameAsString() == "main")
        {
            this->entryFunc = fun;
            return;
        }
    }
    this->entryFunc = nullptr;
    return;
}

std::vector<Defect> LoopAnalyze::check()
{
    getEntryFunc();
    //TODO

    std::vector<Defect> result; 


    return result;
}