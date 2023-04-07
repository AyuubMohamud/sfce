#pragma once
#include <sfce.hh>
#include <cparse.hh>
#include <errorHandler.hh>
class SemanticAnalyser {
public:
    SemanticAnalyser();
    ~SemanticAnalyser();

    bool startSemanticAnalysis(CParse& parserState);
    ScopeAST* scope = nullptr;

    bool analyseFunction(FunctionAST* function);

    bool analyseTree(ASTNode *root);
};