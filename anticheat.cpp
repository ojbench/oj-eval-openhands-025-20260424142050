#include <cstdlib>
#include <iostream>
#include <sstream>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>

#include "lang.h"
#include "visitor.h"

class StructureHash : public Visitor<std::string> {
 public:
  std::string visitProgram(Program *node) override {
    std::string h = "P{";
    for (auto func : node->body) {
      h += visitFunctionDeclaration(func) + ";";
    }
    return h + "}";
  }
  
  std::string visitFunctionDeclaration(FunctionDeclaration *node) override {
    return "F(" + std::to_string(node->params.size()) + ")" + visitStatement(node->body);
  }

  std::string visitExpressionStatement(ExpressionStatement *node) override {
    return "E[" + visitExpression(node->expr) + "]";
  }
  
  std::string visitSetStatement(SetStatement *node) override {
    return "S=" + visitExpression(node->value);
  }
  
  std::string visitIfStatement(IfStatement *node) override {
    return "I?" + visitExpression(node->condition) + ":" + visitStatement(node->body);
  }
  
  std::string visitForStatement(ForStatement *node) override {
    return "L(" + visitStatement(node->init) + ";" + visitExpression(node->test) + ";" + 
           visitStatement(node->update) + ")" + visitStatement(node->body);
  }
  
  std::string visitBlockStatement(BlockStatement *node) override {
    std::string h = "{";
    for (auto stmt : node->body) {
      h += visitStatement(stmt) + ";";
    }
    return h + "}";
  }
  
  std::string visitReturnStatement(ReturnStatement *node) override {
    return "R" + visitExpression(node->value);
  }

  std::string visitIntegerLiteral(IntegerLiteral *node) override {
    return "N";
  }
  
  std::string visitVariable(Variable *node) override {
    return "V";
  }
  
  std::string visitCallExpression(CallExpression *node) override {
    std::string h = "C(" + node->func + ",";
    for (auto expr : node->args) {
      h += visitExpression(expr) + ",";
    }
    return h + ")";
  }
};

class DetailedMetrics : public Visitor<int> {
 public:
  int totalNodes = 0;
  int ifCount = 0;
  int forCount = 0;
  int callCount = 0;
  int setCount = 0;
  std::set<std::string> funcNames;
  std::map<std::string, int> callFreq;
  
  int visitProgram(Program *node) override {
    for (auto func : node->body) {
      funcNames.insert(func->name);
      visitFunctionDeclaration(func);
    }
    return totalNodes;
  }
  
  int visitFunctionDeclaration(FunctionDeclaration *node) override {
    totalNodes++;
    visitStatement(node->body);
    return 0;
  }

  int visitExpressionStatement(ExpressionStatement *node) override {
    totalNodes++;
    return visitExpression(node->expr);
  }
  
  int visitSetStatement(SetStatement *node) override {
    totalNodes++;
    setCount++;
    return visitExpression(node->value);
  }
  
  int visitIfStatement(IfStatement *node) override {
    totalNodes++;
    ifCount++;
    visitExpression(node->condition);
    visitStatement(node->body);
    return 0;
  }
  
  int visitForStatement(ForStatement *node) override {
    totalNodes++;
    forCount++;
    visitStatement(node->init);
    visitExpression(node->test);
    visitStatement(node->update);
    visitStatement(node->body);
    return 0;
  }
  
  int visitBlockStatement(BlockStatement *node) override {
    totalNodes++;
    for (auto stmt : node->body) {
      visitStatement(stmt);
    }
    return 0;
  }
  
  int visitReturnStatement(ReturnStatement *node) override {
    totalNodes++;
    visitExpression(node->value);
    return 0;
  }

  int visitIntegerLiteral(IntegerLiteral *node) override {
    totalNodes++;
    return 0;
  }
  
  int visitVariable(Variable *node) override {
    totalNodes++;
    return 0;
  }
  
  int visitCallExpression(CallExpression *node) override {
    totalNodes++;
    callCount++;
    callFreq[node->func]++;
    for (auto expr : node->args) {
      visitExpression(expr);
    }
    return 0;
  }
};

double calculateSimilarity(Program *prog1, Program *prog2) {
  StructureHash sh;
  std::string hash1 = sh.visitProgram(prog1);
  std::string hash2 = sh.visitProgram(prog2);
  
  DetailedMetrics m1, m2;
  m1.visitProgram(prog1);
  m2.visitProgram(prog2);
  
  double structureSim = 0.0;
  size_t minLen = std::min(hash1.length(), hash2.length());
  size_t maxLen = std::max(hash1.length(), hash2.length());
  if (maxLen > 0) {
    size_t matches = 0;
    for (size_t i = 0; i < minLen; i++) {
      if (hash1[i] == hash2[i]) matches++;
    }
    structureSim = (double)matches / maxLen;
  }
  
  double nodeDiff = abs(m1.totalNodes - m2.totalNodes);
  double nodeRatio = 1.0 - std::min(1.0, nodeDiff / std::max(m1.totalNodes, m2.totalNodes));
  
  double controlDiff = abs(m1.ifCount - m2.ifCount) + abs(m1.forCount - m2.forCount);
  double controlRatio = 1.0 - std::min(1.0, controlDiff / 20.0);
  
  double funcSim = 0.0;
  if (!m1.funcNames.empty() || !m2.funcNames.empty()) {
    std::set<std::string> intersection;
    std::set_intersection(m1.funcNames.begin(), m1.funcNames.end(),
                         m2.funcNames.begin(), m2.funcNames.end(),
                         std::inserter(intersection, intersection.begin()));
    funcSim = (double)intersection.size() / std::max(m1.funcNames.size(), m2.funcNames.size());
  }
  
  double callSim = 0.0;
  if (!m1.callFreq.empty() || !m2.callFreq.empty()) {
    int commonCalls = 0;
    int totalCalls = 0;
    for (auto &p : m1.callFreq) {
      totalCalls += p.second;
      if (m2.callFreq.count(p.first)) {
        commonCalls += std::min(p.second, m2.callFreq[p.first]);
      }
    }
    for (auto &p : m2.callFreq) {
      totalCalls += p.second;
    }
    if (totalCalls > 0) {
      callSim = (double)(2 * commonCalls) / totalCalls;
    }
  }
  
  double similarity = 0.3 * structureSim + 0.2 * nodeRatio + 0.2 * controlRatio + 
                     0.15 * funcSim + 0.15 * callSim;
  
  return 0.5 + 0.5 * similarity;
}

int main() {
  Program *prog1 = scanProgram(std::cin);
  Program *prog2 = scanProgram(std::cin);
  
  std::string input;
  int c;
  while ((c = std::cin.get()) != EOF) {
    input += c;
  }

  double similarity = calculateSimilarity(prog1, prog2);
  similarity = std::max(0.0, std::min(1.0, similarity));
  
  std::cout << similarity << std::endl;
  return 0;
}
