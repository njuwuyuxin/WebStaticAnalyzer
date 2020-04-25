#ifndef CALL_GRAPH_H
#define CALL_GRAPH_H

#include <unordered_map>

#include "ASTManager.h"

class CallGraphNode {
public:
  CallGraphNode(ASTFunction *F) { this->F = F; }

  void addParent(ASTFunction *F) { parents.push_back(F); }

  void addChild(ASTFunction *F) { children.push_back(F); }

  ASTFunction *getFunction() const { return F; }

  const std::vector<ASTFunction *> &getParents() const { return parents; }

  const std::vector<ASTFunction *> &getChildren() const { return children; }
  /* @brief      输出该节点的信息
     @param out  输出流
  */
  void printCGNode(std::ostream& os);

private:
  friend class NonRecursiveCallGraph;

  ASTFunction *F;

  std::vector<ASTFunction *> parents;
  std::vector<ASTFunction *> children;
};

class CallGraph {
public:
  CallGraph(ASTManager &manager, const ASTResource &resource);
  ~CallGraph();

  const std::vector<ASTFunction *> &getTopLevelFunctions() const;

  ASTFunction *getFunction(FunctionDecl *FD) const;

  const std::vector<ASTFunction *> &getParents(ASTFunction *F) const;
  const std::vector<ASTFunction *> &getChildren(ASTFunction *F) const;
  /* @brief      以 .dot文件的形式输出函数调用图
     @param out  输出流
  */
  void writeDotFile(std::ostream& out);
  /* @brief      在控制台中输出函数调用图
     @param out  输出流
  */
  void printCallGraph(std::ostream& out);
  /* @brief      往 .dot文件中写入CGNode信息
     @param out  输出流
  */
  void writeNodeDot(std::ostream& out, CallGraphNode* node);

protected:
  std::unordered_map<std::string, CallGraphNode *> nodes;
  std::vector<ASTFunction *> topLevelFunctions;
  CallGraphNode *getNode(ASTFunction *f);
};

#endif
