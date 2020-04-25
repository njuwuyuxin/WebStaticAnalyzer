#ifndef AST_ELEMENT_H
#define AST_ELEMENT_H

#include <string>
#include <vector>

#include <clang/Frontend/ASTUnit.h>

#include "Common.h"

using namespace clang;

class ASTFunction;
class ASTVariable;

class ASTFile {
public:
  ASTFile(unsigned id, std::string AST) : id(id), AST(AST){};

  const std::string &getAST() const { return AST; }

  void addFunction(ASTFunction *F) { functions.push_back(F); }

  const std::vector<ASTFunction *> &getFunctions() const { return functions; }

private:
  unsigned id;
  std::string AST;

  std::vector<ASTFunction *> functions;
};

class ASTElement {
public:
  ASTElement(unsigned id, std::string name, ASTFile *AF)
      : id(id), name(name), AF(AF) {}

  unsigned getID() const { return id; }

  const std::string &getName() const { return name; }

  ASTFile *getASTFile() const { return AF; }

  const std::string &getAST() const { return AF->getAST(); }

protected:
  unsigned id;
  std::string name;

  ASTFile *AF;
};

class ASTFunction : public ASTElement {
public:
  ASTFunction(unsigned id, FunctionDecl *FD, ASTFile *AF, bool use = true)
      : ASTElement(id, FD->getNameAsString(), AF) {

    this->use = use;

    fullName = common::getFullName(FD);
    param_size = FD->param_size();
  }

  void addVariable(ASTVariable *V) { variables.push_back(V); }

  unsigned getParamSize() const { return param_size; }

  const std::string &getFullName() const { return fullName; }

  const std::vector<ASTVariable *> &getVariables() const { return variables; }

  bool isUse() const { return use; }

private:
  std::string fullName;
  unsigned param_size;

  bool use;

  std::vector<ASTVariable *> variables;
};

class ASTVariable : public ASTElement {
public:
  ASTVariable(unsigned id, VarDecl *VD, ASTFunction *F)
      : ASTElement(id, VD->getNameAsString(), F->getASTFile()), F(F) {

    if (VD->getType()->isPointerType() || VD->getType()->isReferenceType())
      pointer_reference_type = true;
    else
      pointer_reference_type = false;
  }

  ASTFunction *getFunction() const { return F; }

  bool isPointerOrReferenceType() const { return pointer_reference_type; }

private:
  bool pointer_reference_type;

  ASTFunction *F;
};

#endif
