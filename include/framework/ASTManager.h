#ifndef AST_MANAGER_H
#define AST_MANAGER_H

#include <list>
#include <unordered_map>

#include <clang/Analysis/CFG.h>
#include <clang/Frontend/ASTUnit.h>

#include "ASTElement.h"
#include "Config.h"

using namespace clang;

/**
 * the resource of AST.
 * contains AST, function, variables.
 */
class ASTResource {
public:
  ~ASTResource();

  const std::vector<ASTFunction *> &getFunctions(bool use = true) const;
  std::vector<ASTFile *> getASTFiles() const;
  std::vector<EnumDecl *> getEnums() const;
  std::vector<VarDecl *> getVarDecl() const;

  friend class ASTManager;

private:
  std::unordered_map<std::string, ASTFile *> ASTs;

  std::vector<ASTFunction *> ASTFunctions;
  std::vector<ASTVariable *> ASTVariables;
  std::vector<EnumDecl *> Enums;
  std::vector<VarDecl *> VarDecls;

  std::vector<ASTFunction *> useASTFunctions;

  void buildUseFunctions();

  ASTFile *addASTFile(std::string AST);
  ASTFunction *addASTFunction(FunctionDecl *FD, ASTFile *AF, bool use = true);
  ASTVariable *addASTVariable(VarDecl *VD, ASTFunction *F);
  void addEnumDecl(EnumDecl* ED);
  void addVarDecl(VarDecl* VD);
};

/**
 * a bidirectional map.
 * You can get a pointer from an id or get an id from a pointer.
 */
class ASTBimap {
public:
  friend class ASTManager;

private:
  void insertFunction(ASTFunction *F, FunctionDecl *FD);
  void insertVariable(ASTVariable *V, VarDecl *VD);

  FunctionDecl *getFunctionDecl(ASTFunction *F);

  ASTVariable *getASTVariable(VarDecl *VD);
  VarDecl *getVarDecl(ASTVariable *V);

  void removeFunction(ASTFunction *F);
  void removeVariable(ASTVariable *V);

  std::unordered_map<ASTFunction *, FunctionDecl *> functionMap;

  std::unordered_map<ASTVariable *, VarDecl *> variableLeft;
  std::unordered_map<VarDecl *, ASTVariable *> variableRight;
};

/**
 * a class that manages all ASTs.
 */
class ASTManager {
public:
  ASTManager(std::vector<std::string> &ASTs, ASTResource &resource,
             Config &configure);

  ASTUnit *getASTUnit(ASTFile *AF);
  FunctionDecl *getFunctionDecl(ASTFunction *F);

  ASTVariable *getASTVariable(VarDecl *VD);
  VarDecl *getVarDecl(ASTVariable *V);

  std::unique_ptr<CFG> &getCFG(ASTFunction *F);

private:
  ASTResource &resource;
  Config &c;

  ASTBimap bimap;
  std::unordered_map<std::string, ASTUnit *> ASTs;
  std::unordered_map<ASTFunction *, std::unique_ptr<CFG>> CFGs;

  unsigned max_size;
  std::list<std::unique_ptr<ASTUnit>> ASTQueue;

  void pop();
  void move(ASTUnit *AU);
  void push(std::unique_ptr<ASTUnit> AU);

  void loadASTUnit(std::unique_ptr<ASTUnit> AU);
};

#endif
