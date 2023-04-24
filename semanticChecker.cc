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
        case A_CALL:
        {
            i64 pos = scope->findRegularSymbol(node->left->identifier);
            if (pos == -1)
                return true;
            auto* funcSym = parserState.globalSymbolTable.at(pos);
            if (funcSym->type->isPtr() || funcSym->type->isNumVar())
                return true;

            std::vector<CType*> arguments = genArgs(parserState, node->right);
            auto* calleePrototype = dynamic_cast<FunctionPrototype*>(funcSym->type->declaratorPartList.at(1));
            if (arguments.size() != calleePrototype->types.size())
                return true;
            bool error = false;
            for (auto i = 0; i < arguments.size(); i++)
            {
                auto* symbol1 = arguments.at(i);
                auto* symbol2 = calleePrototype->types.at(i);
                bool sym2isPtr = symbol2->type->isPtr();
                if (symbol1 == nullptr)
                    return true;
                bool ok = symbol1->isEqual(symbol2->type, !(symbol1->isPtr()||symbol2->type->isPtr()));
                if (!ok)
                    return true;
            }
            return false;
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
            if (returnExprType->isEqual(currentFunction->funcType(), !returnExprType->isPtr()))
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
            if (ASTopIsBinOp(node->op))
            {
                print_warning(0, currentFunction->funcIdentifier.c_str(), ErrorType::USELESS_EXPRESSION);
            }
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
        if (LHS->isPtr()) {
            if (RHS->declaratorPartList.empty()) // means it is an integer literal
            {
                return LHS;
            }
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
        return normaliseTypes(typeLHS, typeRHS);
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
    if (expr->op == A_LITERAL)
    {
        auto* type = new CType;
        type->typeSpecifier.push_back({
            .token = CHAR,
            .lexeme = "",
            .lineNumber = 0
        });
        auto* pointer = new Pointer;
        pointer->setConst();
        type->declaratorPartList.push_back(pointer);
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
    if (expr->op == A_CALL)
    {
        bool error = analyseTree(parserState, expr);
        if (error)
        {
            return nullptr;
        }
        else {
            i64 pos = scope->findRegularSymbol(expr->left->identifier);
            if (pos == -1)
            {
                printf("Undeclared variable used in file!\n");
                return nullptr;
            }
            return parserState.globalSymbolTable[pos]->type;
        }
    }
    return nullptr;
}

std::vector<CType *> SemanticAnalyser::genArgs(CParse &parserState, ASTNode *argNode) {
        if (argNode == nullptr)
            return {};
        std::vector<CType*> temp;
        if (argNode->op == A_GLUE)
            temp.push_back(evalType(parserState, argNode->left));
        else if (argNode->op == A_CALL)
        {
            auto* type = evalType(parserState, argNode->left);
            if (type->declaratorPartList.size() > 1)
            {
                auto* copyType = new CType;
                copyType->typeSpecifier = type->typeSpecifier;
                for (auto x : type->declaratorPartList)
                {
                    if (x->getDPT() != FUNC)
                    {
                        copyType->declaratorPartList.push_back(x);
                    }
                }
                temp.push_back(copyType);
            }
            return temp;
        }
        else
            temp.push_back(evalType(parserState, argNode));
        auto genArgs2 = genArgs(parserState, argNode->right);
        temp.insert(temp.end(), genArgs2.begin(), genArgs2.end());
        return temp;

}
