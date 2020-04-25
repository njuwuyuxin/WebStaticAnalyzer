#ifndef BASIC_CHECKER_H
#define BASIC_CHECKER_H

#include "ASTManager.h"
#include "CallGraph.h"
#include "Config.h"

class BasicChecker {
public:
  static ASTResource *resource;
  static ASTManager *manager;
  static CallGraph *call_graph;
  static Config *configure;

  BasicChecker(ASTResource *resource, ASTManager *manager,
               CallGraph *call_graph, Config *configure);

  void check();
};

#endif
