#include <iostream>
#include <cstring>
#include <random>
#include <sstream>
#include <set>
#include <map>
#include <algorithm>
#include <cmath>

#include "lang.h"
#include "transform.h"
#include "visitor.h"

// Cheat implementation
class Cheat : public Transform {
 private:
  std::mt19937 rng;
  int counter;
  
 public:
  Cheat() : rng(42), counter(0) {}
  
  Variable *transformVariable(Variable *node) override {
    std::string newName = "v" + std::to_string(std::hash<std::string>{}(node->name) % 10000) + "_" + node->name;
    return new Variable(newName);
  }
  
  FunctionDeclaration *transformFunctionDeclaration(FunctionDeclaration *node) override {
    std::vector<Variable *> params;
    for (auto param : node->params) {
      params.push_back(transformVariable(param));
    }
    std::string newName = node->name;
    if (node->name != "main") {
      newName = "func_" + std::to_string(std::hash<std::string>{}(node->name) % 10000) + "_" + node->name;
    }
    return new FunctionDeclaration(newName, params, transformStatement(node->body));
  }
  
  Expression *transformIntegerLiteral(IntegerLiteral *node) override {
    if (node->value == 0) {
      return new CallExpression("-", {new IntegerLiteral(1), new IntegerLiteral(1)});
    } else if (node->value == 1) {
      return new CallExpression("+", {new IntegerLiteral(0), new IntegerLiteral(1)});
    } else if (node->value > 1 && node->value < 100) {
      return new CallExpression("+", {new IntegerLiteral(node->value - 1), new IntegerLiteral(1)});
    }
    return new IntegerLiteral(node->value);
  }
  
  Statement *transformIfStatement(IfStatement *node) override {
    auto cond = transformExpression(node->condition);
    auto body = transformStatement(node->body);
    auto dummyVar = new Variable("dummy_" + std::to_string(counter++));
    std::vector<Statement*> stmts;
    stmts.push_back(new SetStatement(dummyVar, new IntegerLiteral(0)));
    stmts.push_back(new IfStatement(cond, body));
    return new BlockStatement(stmts);
  }
  
  Statement *transformBlockStatement(BlockStatement *node) override {
    std::vector<Statement *> body;
    for (auto stmt : node->body) {
      body.push_back(transformStatement(stmt));
      if (rng() % 3 == 0) {
        auto dummyVar = new Variable("nop_" + std::to_string(counter++));
        body.push_back(new SetStatement(dummyVar, new IntegerLiteral(0)));
      }
    }
    return new BlockStatement(body);
  }
};

// Anticheat implementation
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

int main(int argc, char *argv[]) {
  // Default to cheat mode for now
  bool cheatMode = true;
  
  if (argc > 1) {
    if (strcmp(argv[1], "anticheat") == 0 || strcmp(argv[1], "-a") == 0) {
      cheatMode = false;
    }
  }
  
  if (cheatMode) {
    auto code = scanProgram(std::cin);
    auto cheat = Cheat().transformProgram(code);
    std::cout << cheat->toString();
  } else {
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
  }
  
  return 0;
}
