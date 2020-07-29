#ifndef BASE_COMMON_H
#define BASE_COMMON_H

#include <vector>
#include <unordered_map>
#include <string>

#include <clang/Frontend/ASTUnit.h>

#include "Config.h"

using namespace clang;
using namespace std;

std::vector<std::string> initialize(std::string astList);

namespace common {

enum CheckerName {
  taintChecker,
  danglingPointer,
  arrayBound,
  recursiveCall,
  divideChecker,
  memoryOPChecker
};

std::unique_ptr<ASTUnit> loadFromASTFile(std::string AST);

std::vector<FunctionDecl *> getFunctions(ASTContext &Context,SourceLocation SL);
std::unordered_map<string,EnumDecl *> getEnums(ASTContext &Context,SourceLocation SL);
std::vector<VarDecl *> getVarDecl(ASTContext &Context);
std::vector<VarDecl *> getVariables(FunctionDecl *FD);

std::vector<FunctionDecl *> getCalledFunctions(FunctionDecl *FD);
std::vector<CallExpr *> getCallExpr(FunctionDecl *FD);

std::string getFullName(FunctionDecl *FD);

void printLog(std::string, CheckerName cn, int level, Config &c);

template <class T> void dumpLog(T &t, CheckerName cn, int level, Config &c) {
  auto block = c.getOptionBlock("PrintLog");
  int l = atoi(block.find("level")->second.c_str());
  switch (cn) {
  case common::CheckerName::taintChecker:
    if (block.find("taintChecker")->second == "true" && level >= l) {
      t.dump();
    }
    break;
  case common::CheckerName::danglingPointer:
    if (block.find("danglingPointer")->second == "true" && level >= l) {
      t.dump();
    }
    break;
  case common::CheckerName::arrayBound:
    if (block.find("arrayBound")->second == "true" && level >= l) {
      t.dump();
    }
    break;
  case common::CheckerName::recursiveCall:
    if (block.find("recursiveCall")->second == "true" && level >= l) {
      t.dump();
    }
    break;
  case common::CheckerName::divideChecker:
    if (block.find("divideChecker")->second == "true" && level >= l) {
      t.dump();
    }
    break;
  case common::CheckerName::memoryOPChecker:
    if (block.find("memoryOPChecker")->second == "true" && level >= l) {
      t.dump();
    }
    break;
  }
}

} // end of namespace common

#endif
