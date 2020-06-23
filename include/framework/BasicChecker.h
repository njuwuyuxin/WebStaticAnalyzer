#ifndef BASIC_CHECKER_H
#define BASIC_CHECKER_H

#include <string>
#include <vector>
#include "ASTManager.h"
#include "CallGraph.h"
#include "Config.h"

struct Defect {
  std::string location;
  std::string info;
};

class BasicChecker {
public:
  static ASTResource *resource;
  static ASTManager *manager;
  static CallGraph *call_graph;
  static Config *configure;

  BasicChecker(ASTResource *resource, ASTManager *manager,
               CallGraph *call_graph, Config *configure);

  virtual std::vector<Defect> check() = 0;
  virtual void report(Expr *expr, int level) {}
};

#endif
