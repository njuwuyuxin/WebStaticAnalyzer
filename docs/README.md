# SE-Experiment 项目文档

## Framework提供的内容（BasicChecker都包含）


1.  ASTManager manager:

    1. `FunctionDecl *getFunctionDecl(ASTFunction *F)`：通过ASTFunction（该Framework的函数结构）获得FD（Clang的函数结构）

    2. `ASTVariable *getASTVariable(VarDecl *VD)`，`VarDecl *getVarDecl(ASTVariable *V)`：VarDecl和ASTVariable互相转换

    4. `std::unique_ptr<CFG> &getCFG(ASTFunction *F)`：通过ASTFunction得到CFG，其实`std::unique_ptr<CFG> functionCFG = std::unique_ptr<CFG>(CFG::buildCFG(
      FD, FD->getBody(), &FD->getASTContext(), CFG::BuildOptions()));`也可以，其中FD是FunctionDecl类型

2. CallGraph call_graph：
    
    1. `std::vector<ASTFunction *> &getTopLevelFunctions()`：分析出所有顶层函数，即没有调用者的函数（例如main）

    2.  `ASTFunction *getFunction(FunctionDecl *FD) `：通过FD（Clang的函数结构）获得ASTFunction（该Framework的函数结构）

    3.  `std::vector<ASTFunction *> &getParents(ASTFunction *F)`, `std::vector<ASTFunction *> &getChildren(ASTFunction *F)`：获得某个函数的被调用者和调用者

3. Config configure: 用于读取配置文件。具体使用可以参`TemplateChecker::readConfig().`

## BasicChecker

简单来说，BasicChecker就是存储了framework提供的类，让我们在分析的时候可以调用。它的所有子类，都可以基于该framework提供的功能进行分析。虽然名字是关于checker，但是也可以用于分析。

