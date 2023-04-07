//
// Created by ayuubmohamud on 03/04/23.
//
#include "semanticChecker.hh"
#include <errorHandler.hh>
SemanticAnalyser::SemanticAnalyser() {

}

SemanticAnalyser::~SemanticAnalyser() {

}
bool SemanticAnalyser::analyseTree(ASTNode* root)
{
    switch (root->op) {
        case A_CS:
        {

        }
        case A_RET:
        {

        }
        case A_MV:
        {

        }
        case A_GLUE:
        {

        }
        default:
        {
        }
    }
    int x = 0;
}

bool SemanticAnalyser::analyseFunction(FunctionAST* function) {

}

bool SemanticAnalyser::startSemanticAnalysis(CParse& parserState) {
    scope = parserState.currentScope;
    for (auto* i : parserState.functions) {
        bool error = analyseFunction(i);
        if (error)
            return false;
    }
    return false;
}
