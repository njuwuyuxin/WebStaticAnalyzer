#include "framework/ASTManager.h"
#include "framework/Common.h"

#include <clang/Frontend/CompilerInstance.h>

const std::vector<ASTFunction *> &ASTResource::getFunctions(bool use) const {
  if (use) {
    return useASTFunctions;
  }

  return ASTFunctions;
}

std::vector<ASTFile *> ASTResource::getASTFiles() const {
  std::vector<ASTFile *> ASTFiles;
  for (auto &it : ASTs) {
    ASTFiles.push_back(it.second);
  }
  return ASTFiles;
}

std::unordered_map<string,EnumDecl*> ASTResource::getEnums() const {
  cout<<"in ASTManager getEnums: start dump all"<<endl;
  cout<<"enums size="<<Enums.size()<<endl;
  for(auto i:Enums){
    i.second->dump();
  }
  return Enums;
}

std::vector<VarDecl *> ASTResource::getVarDecl() const{
  return VarDecls;
}

void ASTResource::buildUseFunctions() {
  for (ASTFunction *AF : ASTFunctions) {
    if (AF->isUse()) {
      useASTFunctions.push_back(AF);
    }
  }
}

ASTFile *ASTResource::addASTFile(std::string AST) {
  unsigned id = ASTs.size();
  ASTFile *AF = new ASTFile(id, AST);
  ASTs[AST] = AF;
  return AF;
}

ASTFunction *ASTResource::addASTFunction(FunctionDecl *FD, ASTFile *AF,
                                         bool use) {
  unsigned id = ASTFunctions.size();
  ASTFunction *F = new ASTFunction(id, FD, AF, use);
  ASTFunctions.push_back(F);
  AF->addFunction(F);
  return F;
}

ASTVariable *ASTResource::addASTVariable(VarDecl *VD, ASTFunction *F) {
  unsigned id = F->getVariables().size();
  ASTVariable *V = new ASTVariable(id, VD, F);
  ASTVariables.push_back(V);
  F->addVariable(V);
  return V;
}

void ASTResource::addEnumDecl(EnumDecl* ED){
  string EDname = ED->getName();
  if(Enums.find(EDname)!=Enums.end()){
    cout<<"The same Enum defination!"<<endl;
    cout<<"this enum is "<<EDname<<endl;
    // return;
  }
  Enums[ED->getName()]=ED;
}

void ASTResource::addVarDecl(VarDecl* VD){
  VarDecls.push_back(VD);
}

ASTResource::~ASTResource() {
  for (auto &content : ASTs) {
    delete content.second;
  }
  for (ASTFunction *F : ASTFunctions) {
    delete F;
  }
  for (ASTVariable *V : ASTVariables) {
    delete V;
  }
}

void ASTBimap::insertFunction(ASTFunction *F, FunctionDecl *FD) {
  functionMap[F] = FD;
}

void ASTBimap::insertVariable(ASTVariable *V, VarDecl *VD) {
  variableLeft[V] = VD;
  variableRight[VD] = V;
}

FunctionDecl *ASTBimap::getFunctionDecl(ASTFunction *F) {
  auto it = functionMap.find(F);
  if (it == functionMap.end()) {
    return nullptr;
  }
  return it->second;
}

ASTVariable *ASTBimap::getASTVariable(VarDecl *VD) {
  auto it = variableRight.find(VD);
  if (it == variableRight.end()) {
    return nullptr;
  }
  return it->second;
}

VarDecl *ASTBimap::getVarDecl(ASTVariable *V) {
  auto it = variableLeft.find(V);
  if (it == variableLeft.end()) {
    return nullptr;
  }
  return it->second;
}

void ASTBimap::removeFunction(ASTFunction *F) { functionMap.erase(F); }

void ASTBimap::removeVariable(ASTVariable *V) {
  VarDecl *VD = getVarDecl(V);
  variableLeft.erase(V);
  variableRight.erase(VD);
}

ASTManager::ASTManager(std::vector<std::string> &ASTs, ASTResource &resource,
                       Config &configure)
    : resource(resource), c(configure) {
  max_size = std::stoi(configure.getOptionBlock("Framework")["queue_size"]);
  std::unordered_set<std::string> functionNames;
  for (std::string AST : ASTs) {

    ASTFile *AF = resource.addASTFile(AST);
    std::unique_ptr<ASTUnit> AU = common::loadFromASTFile(AST);
    std::vector<FunctionDecl *> functions =
        common::getFunctions(AU->getASTContext(),AU->getStartOfMainFileID());

    for (FunctionDecl *FD : functions) {
      std::string name = common::getFullName(FD);
      bool use = (functionNames.count(name) == 0);
      if (use == true) {
        functionNames.insert(name);
      }

      ASTFunction *F = resource.addASTFunction(FD, AF, use);
      std::vector<VarDecl *> variables = common::getVariables(FD);

      for (VarDecl *VD : variables) {
        resource.addASTVariable(VD, F);
      }
    }

    std::unordered_map<string,EnumDecl *> enums =
        common::getEnums(AU->getASTContext(),AU->getStartOfMainFileID());
    for(auto ED: enums){
      resource.addEnumDecl(ED.second);
    }

    std::vector<VarDecl *> vars = 
        common::getVarDecl(AU->getASTContext());
    for(VarDecl *VD: vars){
      resource.addVarDecl(VD);
    }
    
    loadASTUnit(std::move(AU));
  }
  resource.buildUseFunctions();
}

void ASTManager::loadASTUnit(std::unique_ptr<ASTUnit> AU) {
  if (ASTQueue.size() == max_size) {
    pop();
  }
  push(std::move(AU));
}

ASTUnit *ASTManager::getASTUnit(ASTFile *AF) {
  auto it = ASTs.find(AF->getAST());
  if (it == ASTs.end()) {
    loadASTUnit(common::loadFromASTFile(AF->getAST()));
  } else {
    ASTUnit *AU = it->second;
    move(AU);
  }
  return ASTs[AF->getAST()];
}

FunctionDecl *ASTManager::getFunctionDecl(ASTFunction *F) {
  if (F == nullptr) {
    return nullptr;
  }

  FunctionDecl *FD = bimap.getFunctionDecl(F);
  if (FD != nullptr) {
    move(ASTs[F->getAST()]);
  } else {
    loadASTUnit(common::loadFromASTFile(F->getAST()));
    FD = bimap.getFunctionDecl(F);
  }
  return FD;
}

ASTVariable *ASTManager::getASTVariable(VarDecl *VD) {
  return bimap.getASTVariable(VD);
}

VarDecl *ASTManager::getVarDecl(ASTVariable *V) {
  if (V == nullptr) {
    return nullptr;
  }

  VarDecl *VD = bimap.getVarDecl(V);
  if (VD == nullptr) {
    loadASTUnit(common::loadFromASTFile(V->getAST()));
    VD = bimap.getVarDecl(V);
  }
  return VD;
}

std::unique_ptr<CFG> &ASTManager::getCFG(ASTFunction *F) {
  auto it = CFGs.find(F);
  if (it != CFGs.end()) {
    move(ASTs[F->getAST()]);
    return it->second;
  }

  FunctionDecl *FD = getFunctionDecl(F);

  std::unique_ptr<CFG> functionCFG = std::unique_ptr<CFG>(CFG::buildCFG(
      FD, FD->getBody(), &FD->getASTContext(), CFG::BuildOptions()));

  return CFGs[F] = std::move(functionCFG);
}

/** move ASTUnit to the end of the queue
 **/
void ASTManager::move(ASTUnit *AU) {
  std::unique_ptr<ASTUnit> NAU;
  auto it = ASTQueue.begin();
  for (; it != ASTQueue.end(); it++) {
    if ((*it).get() == AU) {
      NAU = std::move(*it);
      break;
    }
  }
  assert(it != ASTQueue.end());
  ASTQueue.erase(it);
  ASTQueue.push_back(std::move(NAU));
}

/** pop a ASTUnit in the front of the queue
 **/
void ASTManager::pop() {
  std::string AST = ASTQueue.front()->getASTFileName();
  for (ASTFunction *F : resource.ASTs[AST]->getFunctions()) {
    for (ASTVariable *V : F->getVariables())
      bimap.removeVariable(V);
    bimap.removeFunction(F);
    CFGs.erase(F);
  }

  ASTs.erase(AST);
  ASTQueue.pop_front();

  common::printLog("pop" + AST + "\n", common::CheckerName::taintChecker, 1, c);
}

void ASTManager::push(std::unique_ptr<ASTUnit> AU) {
  std::string AST = AU->getASTFileName();

  const std::vector<FunctionDecl *> &functions =
      common::getFunctions(AU->getASTContext(),AU->getStartOfMainFileID());
  const std::vector<ASTFunction *> &ASTFunctions =
      resource.ASTs[AST]->getFunctions();

  for (unsigned i = 0; i < functions.size(); i++) {
    FunctionDecl *FD = functions[i];
    ASTFunction *F = ASTFunctions[i];
    bimap.insertFunction(F, FD);

    const std::vector<VarDecl *> &variables = common::getVariables(FD);
    const std::vector<ASTVariable *> &ASTVariables = F->getVariables();

    for (unsigned j = 0; j < variables.size(); j++) {
      bimap.insertVariable(ASTVariables[j], variables[j]);
    }
  }

  ASTs[AST] = AU.get();
  ASTQueue.push_back(std::move(AU));

  common::printLog("push" + AST + "\n", common::CheckerName::taintChecker, 1,
                   c);
}
