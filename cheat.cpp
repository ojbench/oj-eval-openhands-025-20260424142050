#include <iostream>
#include <random>
#include <sstream>

#include "lang.h"
#include "transform.h"

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

int main() {
  auto code = scanProgram(std::cin);
  auto cheat = Cheat().transformProgram(code);
  std::cout << cheat->toString();
  return 0;
}
