#include <cparse.hh>
#include <errorHandler.hh>
SemanticAnalyser::SemanticAnalyser() {

}

SemanticAnalyser::~SemanticAnalyser() {

}
bool SemanticAnalyser::analyseTree(CParse& parserState, ASTNode* node)
{
    switch (node->op) {
        case A_CS:
        {
            ScopeAST* temp;
            temp = scope;
            scope = node->scope;
            bool error = analyseTree(parserState, node->left);
            if (error)
            {
                return true;
            }
            else {
                scope = temp;
                return false;
            }
        }
        case A_RET: // check if return value makes sense
        {
            bool deleteRetType = false;
            auto* returnExprType = evalType(parserState, node->left);
            if (returnExprType == nullptr)
            {
                returnExprType = new CType;
                returnExprType->typeSpecifier.push_back({.token = VOID});
                deleteRetType = true;
            }
            i64 pos = scope->findRegularSymbol(currentFunction->funcIdentifier);
            if (pos == -1) {
                if (deleteRetType)
                    delete returnExprType;
                return true;
            }
            if (returnExprType->isEqual(parserState.globalSymbolTable[pos]->type, !returnExprType->isPtr()))
            {
                if (deleteRetType)
                    delete returnExprType;
                return false;
            }
            printf("Function %s does not have return type %s as indicated by return expression\n", currentFunction->funcIdentifier.c_str(), returnExprType->typeAsString().c_str());
            if (deleteRetType)
                delete returnExprType;
            return true;
        }
        case A_MV:
        {
            auto* RHSType = evalType(parserState, node->right);
            auto* LHSType = evalType(parserState, node->left);
            if (RHSType == nullptr || LHSType == nullptr)
                return true;
            if (RHSType->isEqual(LHSType, !RHSType->isPtr()))
            {
                break;
            }
            print_error(0, currentFunction->funcIdentifier.c_str(), RHSType->typeAsString().c_str(), LHSType->typeAsString().c_str());
            return true;
        }
        case A_GLUE:
        {
            return (analyseTree(parserState, node->left) || analyseTree(parserState, node->right));
        }
        case A_END:
        {
            return false;
        }
        default:
        {
        }
    }
    return false;
}

bool SemanticAnalyser::analyseFunction(CParse& parserState, FunctionAST* function) {
    auto* funcPrototype = dynamic_cast<FunctionPrototype*>(parserState.globalSymbolTable[function->globalSymTableIdx]->type->declaratorPartList[1]);
    scope = funcPrototype->scope;
    currentFunction = function;
    return analyseTree(parserState, function->root);
}

bool SemanticAnalyser::startSemanticAnalysis(CParse& parserState) {
    scope = parserState.currentScope;
    for (auto* i : parserState.functions) {
        bool error = analyseFunction(parserState,i);
        if (error)
            return false;
    }
    return true;
}
CType* SemanticAnalyser::normaliseTypes(CType* LHS, CType* RHS) const
{
    if (RHS == nullptr || LHS == nullptr)
        return nullptr;
    if (RHS->isPtr())
    {
        if (LHS->isPtr()) {
            if (RHS->isEqual(LHS, false)) return LHS;
            std::string typeLHS = LHS->typeAsString();
            std::string typeRHS = RHS->typeAsString();
            print_error(0, currentFunction->funcIdentifier.c_str(), typeLHS.c_str(), typeRHS.c_str());
        }
        else return nullptr;
    }

    if (RHS->isNumVar())
    {
        if (LHS->isNumVar())
        {
            return LHS;
        }
        else {
            std::string typeLHS = LHS->typeAsString();
            std::string typeRHS = RHS->typeAsString();
            print_error(0, currentFunction->funcIdentifier.c_str(), typeLHS.c_str(), typeRHS.c_str());
            return nullptr;
        }
    }

    return nullptr;

}

CType* SemanticAnalyser::evalType(CParse& parserState, ASTNode *expr) {
    if (expr == nullptr)
    {
        return nullptr;
    }
    if (ASTopIsBinOp(expr->op))
    {
        auto* typeRHS = evalType(parserState, expr->right);
        auto* typeLHS = evalType(parserState, expr->left);
        return normaliseTypes(typeRHS, typeLHS);
    }
    if (expr->op == A_INTLIT)
    {
        auto* type = new CType;
        type->typeSpecifier.push_back({
            .token = INTEGER,
            .lexeme = "",
            .lineNumber = 0
        });
        expr->type = type;
        return type;
    }
    if (expr->op == A_INC || expr->op == A_DEC)
    {
        auto* typeUnary = evalType(parserState, expr->left);
        if (typeUnary == nullptr)
            return nullptr;
        if (typeUnary->isPtr())
        {
            expr->value = sizeOf(typeUnary->typeSpecifier);
        }
        if (typeUnary->isNumVar() || typeUnary->isPtr())
            return typeUnary;

        print_error(0, currentFunction->funcIdentifier.c_str(), typeUnary->typeAsString().c_str());
        return nullptr;
    }
    if (expr->op == A_DEREF)
    {
        auto* typeUnary = evalType(parserState, expr->left);
        if (typeUnary->isPtr() || typeUnary->isFuncPtr())
        {
            auto* pCType = typeUnary->dereferenceType();
            expr->type = pCType;
            return pCType;
        }
        print_error(0, currentFunction->funcIdentifier.c_str(), typeUnary->typeAsString().c_str());
        return nullptr;
    }
    if (expr->op == A_AGEN)
    {
        auto* typeUnary = evalType(parserState, expr->left);
        if (typeUnary == nullptr)
            return nullptr;
        auto* refType = typeUnary->refType();
        if (refType == nullptr) {
            return nullptr;
        }
        expr->type = refType;
        return refType;
    }
    if (expr->op == A_TYPE_CVT)
    {
        return expr->left->type;
    }
    if (expr->op == A_TYP)
    {
        return expr->type;
    }
    if (expr->op == A_IDENT) {
        i64 pos = scope->findRegularSymbol(expr->identifier);
        if (pos == -1)
        {
            printf("Undeclared variable used in file!\n");
            return nullptr;
        }
        return parserState.globalSymbolTable[pos]->type;
    }
    return nullptr;
}
