#include "framework/BasicChecker.h"

ASTResource *BasicChecker::resource = nullptr;
ASTManager *BasicChecker::manager = nullptr;
CallGraph *BasicChecker::call_graph = nullptr;
Config *BasicChecker::configure = nullptr;

BasicChecker::BasicChecker(ASTResource *resource, ASTManager *manager,
                           CallGraph *call_graph, Config *configure) {
  this->resource = resource;
  this->manager = manager;
  this->call_graph = call_graph;
  this->configure = configure;
}

void BasicChecker::check() {}
