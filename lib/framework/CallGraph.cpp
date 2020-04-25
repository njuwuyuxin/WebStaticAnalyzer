#include "framework/CallGraph.h"

#include <iostream>

using namespace std;

CallGraph::CallGraph(ASTManager &manager, const ASTResource &resource) {
  //为从resource中获取的每一个函数创建相应的CallGraphNode并将其
  //加入nodes中
  for (ASTFunction *F : resource.getFunctions()) {
    CallGraphNode *node = new CallGraphNode(F);
    nodes.insert(std::make_pair(F->getFullName(), node));
  }

  for (auto &content : nodes) {
    //对nodes每一对string与CallGraphNode，获取其CallGraphNode对应ASTFunction
    //的FunctionDecl
    CallGraphNode *node = content.second;
    //获取一个FunctionDecl调用的函数
    FunctionDecl *FD = manager.getFunctionDecl(node->getFunction());
    auto v = common::getCalledFunctions(FD);

    for (FunctionDecl *called_func : v) {
      string name = common::getFullName(called_func);
      auto it = nodes.find(name);
      //加入调用与被调用关系
      if (it != nodes.end()) {
        node->addChild(it->second->getFunction());
        it->second->addParent(node->getFunction());
      }
    }
  }
  //将顶层函数放入topLevelFunctions中
  for (ASTFunction *F : resource.getFunctions()) {
    if (nodes[F->getFullName()]->getParents().size() == 0) {
      topLevelFunctions.push_back(F);
    }
  }
}

CallGraph::~CallGraph() {
  for (auto &content : nodes) {
    delete content.second;
  }
}

const std::vector<ASTFunction *> &CallGraph::getTopLevelFunctions() const {
  return topLevelFunctions;
}

ASTFunction *CallGraph::getFunction(FunctionDecl *FD) const {
  std::string fullName = common::getFullName(FD);
  auto it = nodes.find(fullName);
  if (it != nodes.end()) {
    return it->second->getFunction();
  }
  return nullptr;
}

const std::vector<ASTFunction *> &CallGraph::getParents(ASTFunction *F) const {
  auto it = nodes.find(F->getFullName());
  return it->second->getParents();
}

const std::vector<ASTFunction *> &CallGraph::getChildren(ASTFunction *F) const {
  auto it = nodes.find(F->getFullName());
  return it->second->getChildren();
}

CallGraphNode *CallGraph::getNode(ASTFunction *f) {
  if (f == nullptr) {
    return nullptr;
  }
  auto it = nodes.find(f->getFullName());

  if (it == nodes.end()) {
    return nullptr;
  }
  return it->second;
}
void CallGraphNode::printCGNode(std::ostream& os) {
    std::string functionName = F->getFullName();
    os << "Callee of " << functionName << ". " << std::endl;
    for (ASTFunction* function : children) {
        std::string calleeName = function->getFullName();
        os << " " << calleeName << std::endl;
    }
}
void CallGraph::printCallGraph(std::ostream& out) {
    auto it = nodes.begin();
    for (; it != nodes.end(); it++) {
        CallGraphNode* temp = it->second;
        temp->printCGNode(out);
    }

}
void CallGraph::writeDotFile(std::ostream& out) {
    std::string head = "digraph \"Call graph\" {";
    std::string end = "}";
    std::string label = "    label=\"Call graph\"";
    out << head << std::endl;
    out << label << std::endl << std::endl;
    auto it = nodes.begin();
    for (; it != nodes.end(); it++) {
        CallGraphNode* temp = it->second;
        writeNodeDot(out, temp);
    }
    out << end << std::endl;

}
void CallGraph::writeNodeDot(std::ostream& out, CallGraphNode* node) {
    ASTFunction *function = node->getFunction();
    out << "    Node" << function << " [shape=record,label=\"{";
    out << function->getFullName() << "}\"];" << endl;
    for (ASTFunction* func : node->getChildren()) {
        out << "    Node" << function << " -> " << "Node" << func << endl;
    }
}
