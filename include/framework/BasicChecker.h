#ifndef BASIC_CHECKER_H
#define BASIC_CHECKER_H

#include "ASTManager.h"
#include "CallGraph.h"
#include "Config.h"
#include <string>
#include <vector>

using Defect = std::tuple<std::string, std::string>;
using DefectSet = std::set<Defect>;

class BasicChecker {
public:
  static ASTResource *resource;
  static ASTManager *manager;
  static CallGraph *call_graph;
  static Config *configure;

  BasicChecker(ASTResource *resource, ASTManager *manager,
               CallGraph *call_graph, Config *configure);

  virtual void check() = 0;
  virtual void report(const Expr *expr, int level) {}
  void addDefect(const Defect &d) { defects.insert(d); }
  const DefectSet &getDefects() const { return defects; }
  bool isInSameLocation(const Defect &d){
    for(auto i:defects){
      if(get<0>(i) == get<0>(d)){
        return true;
      }
    }
    return false;
  }

private:
  DefectSet defects;
};

#endif
