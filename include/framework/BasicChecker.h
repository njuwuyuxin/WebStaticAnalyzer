#ifndef BASIC_CHECKER_H
#define BASIC_CHECKER_H

#include "ASTManager.h"
#include "CallGraph.h"
#include "Config.h"
#include <string>
using namespace std;
using std::string;

class BasicChecker {
public:
  static ASTResource *resource;
  static ASTManager *manager;
  static CallGraph *call_graph;
  static Config *configure;
  string name;

  BasicChecker(ASTResource *resource, ASTManager *manager,
               CallGraph *call_graph, Config *configure,string checkername);

  virtual void check();
};

#endif
