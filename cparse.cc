#include "cparse.hh"
#include <errorHandler.hh>

/*
MIT License

Copyright (c) 2023 Ayuub Mohamud

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// Recursive Descent Parser for a subset of C programming language (2011 specification)
std::unordered_map<TokenType, ASTop> binHashMap = {
        {STAR, A_MULT},
        {AMPERSAND, A_AND},
        {LOGICALAND, A_LAND},
        {LOGICALORR, A_LOR},
        {BACKSLASH, A_DIV},
        {MODULO, A_MODULO},
        {ADD, A_ADD},
        {MINUS, A_SUB},
        {LSL, A_SLL},
        {LSR, A_ASR},
        {LESSTHAN, A_LT},
        {LESSTHANOREQUALTO, A_LTEQ},
        {MORETHAN, A_MT},
        {MORETHANOREQUALTO, A_MTEQ},
        {EQUAL, A_EQ},
        {NOTEQUAL, A_NEQ},
        {BITWISEORR, A_OR},
        {BITWISEXOR, A_XOR}
};
bool ASTopIsBinOp(ASTop op) {
    return (
            op == A_MULT
            || op == A_AND
            || op == A_LAND
            || op == A_LOR
            || op == A_DIV
            || op == A_MODULO
            || op == A_ADD
            || op == A_SUB
            || op == A_SLL
            || op == A_ASR
            || op == A_LT
            || op == A_LTEQ
            || op == A_MT
            || op == A_MTEQ
            || op == A_EQ
            || op == A_NEQ
            || op == A_OR
            || op == A_XOR
            );
}
bool ASTopIsCMPOp(ASTop op) {
    return (
            op == A_LT
            || op == A_LTEQ
            || op == A_MT
            || op == A_MTEQ
            || op == A_EQ
            || op == A_NEQ
    );
}
void deleteVecOfPtrs(std::vector<CType*>* ptrs)
{
    for (auto i : *ptrs)
    {
        delete i;
    }
}
void deleteVecOfPtrs(std::vector<Symbol*>* ptrs)
{
    for (auto i : *ptrs)
    {
        delete i;
    }

}
bool isTypeSpecifier(const Token& token)
{
    if ((token.token == INTEGER) || (token.token == CHAR) || (token.token == SHORT) || (token.token == VOID))
    {
        return true;
    }
    return false;
}
bool isTypeQualifier(const Token& token)
{
    if (token.token == CONST || token.token == VOLATILE)
    {
        return true;
    }
    return false;
}
int sizeOf(std::vector<Token>& typeSpecifiers)
{
    bool typeHit = false;
    u64 x = 0;
    Token type;
    while (x != (typeSpecifiers.size() - 1))
    {
        if (isTypeSpecifier(typeSpecifiers[x]))
        {
            type = typeSpecifiers[x];
            break;
        }
        x++;
    }
    switch (type.token)
    {
        case INTEGER: {
            return 4;
        }
        case SHORT: {
            return 2;
        }
        case CHAR:
        {
            return 1;
        }
        case LONG:
        {
            return 8;
        }
        default: {
            return 0;
        }
    }
}

int getPlatformPtrSz() {
    return 8;
}

int determineSz(CType* type)
{
    if (type->declaratorPartList.empty())
    {
        return sizeOf(type->typeSpecifier);
    }
    if (type->declaratorPartList.at(0)->getDPT() == PTR)
    {
        return getPlatformPtrSz();
    }
    return 0;
}
/*
 * unary_operator
 	: '&'
 	| '*'
 	| '+'
 	| '-'
 	| '~'
 	| '!'
 	;*/
ASTop isUnaryOperator(Token& token)
{
    switch (token.token) {
        case AMPERSAND:
        {
            return A_AGEN;
        }
        case STAR:
        {
            return A_DEREF;
        }
        case ADD:
        {
            return A_ABS;
        }
        case MINUS:
        {
            return A_NEG;
        }
        case BITWISENOT:
        {
            return A_NOT;
        }
        case NEGATE:
        {
            return A_LNOT;
        }
        default:
        {
            return A_NOP;
        }
    }
}

CParse::CParse(std::vector<Token>* input)
{
    tokens = input;
}
/*
 *external_declaration
	: function_definition
	| declaration
	;
 declaration
	: declaration_specifiers ';'
	| declaration_specifiers init_declarator_list ';'
	;
 function_definition
 	: declaration_specifiers declarator compound_statement
 	;
 */
bool CParse::parse()
{
    auto* globalScope = new ScopeAST;
    currentScope = globalScope;
    currentScope->parent = nullptr;
    while (tokens->at(cursor).token != END)
    {
        auto* type = new CType;
        if (declarationSpecifiers(type))
        {
            delete type;
            delete currentScope;
            return false;
        }

        if (!initDeclaratorList(type)) {
            print_error("Failure whilst parsing declarator!");
            delete currentScope;
            delete type;
            return false;
        }
        if (tokens->at(cursor).token != SEMICOLON)
        {
            if (tokens->at(cursor).token == OPEN_BRACE) {
                auto* function = new FunctionAST(type, identifier);
                currentFunction = function;
                function->globalSymTableIdx = currentScope->findSymbolInLocalScope(function->funcIdentifier);
                dynamic_cast<FunctionPrototype*>(globalSymbolTable[function->globalSymTableIdx]->type->declaratorPartList.at(1))->scope->parent = currentScope;
                currentScope = dynamic_cast<FunctionPrototype*>(globalSymbolTable[function->globalSymTableIdx]->type->declaratorPartList.at(1))->scope;
                function->root = compoundStatement();
                currentScope = currentScope->parent;
                functions.push_back(function);
            }
            else {
                print_error(tokens->at(cursor).lineNumber, "Expected semicolon after declaration");
                delete currentScope;
                delete type;
                return false;
            }
        }
        else {
            cursor++;
        }
    }
    return true;
}
/*declaration_specifiers
	: type_specifier
	| type_specifier declaration_specifiers
	| type_qualifier
	| type_qualifier declaration_specifiers
	;

type_specifier
	: VOID
	| CHAR
	| SHORT
	| INT
	| LONG
	| SIGNED
	| UNSIGNED
	;
 */
bool CParse::declarationSpecifiers(CType* cType)
{
    while (tokens->at(cursor).token != END && (isTypeQualifier(tokens->at(cursor)) || isTypeSpecifier(tokens->at(cursor))))
    {
        if (!combinable(cType, tokens->at(cursor)))
        {
            print_error("Uncombinable");
            return true;
        }
        cType->typeSpecifier.push_back(tokens->at(cursor));
        cursor++;
    }

    return false;
}

CParse::~CParse() {
    for (auto* i: functions)
    {
        ASTNode::deleteNode(i->root);
        delete i;
    }

    for (auto* i : globalSymbolTable)
    {
        delete i;
    }
    delete currentScope;
}
/*
 * init_declarator_list
	: init_declarator
	;
*/


bool CParse::combinable(CType *cType, const Token& token)
{
    switch (token.token) {
        case SHORT:
        case CHAR:
        case VOID:
        case INTEGER: {
            bool conflictingTypeFound = false;
            for (auto& i: cType->typeSpecifier)
            {
                if (i.token == CHAR || i.token == SHORT || i.token == INTEGER || i.token == VOID) // Add floating point types as well
                    conflictingTypeFound = true;
            }
            if (conflictingTypeFound) {
                return false;
            }
            else return true;
        }
        case STATIC:
        {
        }

        default:
            break;

    }
    return false;
}
/*
compound_statement
	: '{' '}'
	| '{'  block_item_list '}'
	;

 */
ASTNode* CParse::compoundStatement() {
    cursor++;
    auto* node = new ASTNode;
    ASTNode::fillNode(node, nullptr, nullptr, true, A_CS, "");
    if (tokens->at(cursor).token == CLOSE_BRACE)
    {
        return node;
    }
    auto* newScope = new ScopeAST;
    newScope->parent = currentScope;
    currentScope = newScope;
    node->scope = currentScope;
    node->left = blockItemList();
    if (node->left == nullptr) {
        ScopeAST* temp = nullptr;
        temp = currentScope->parent;
        delete currentScope;
        currentScope = temp;
        return nullptr;
    }
    if (tokens->at(cursor).token == CLOSE_BRACE) {
        currentScope = currentScope->parent;
        cursor++;
        return node;
    }
    return nullptr;
}
/*
 *  block_item_list
 	: block_item
 	| block_item_list block_item
 	;
*/
ASTNode* CParse::blockItemList() {
    /*
     * (tokens->at(cursor).token != END && tokens->at(cursor).token != CLOSE_BRACE)*/
    if (tokens->at(cursor).token == CLOSE_BRACE)
    {
        /*
        auto* node = new ASTNode;
        ASTNode::fillNode(node, nullptr, nullptr, false, A_END, "");
        return node;
         */
        auto* node = new ASTNode;
        ASTNode::fillNode(node, nullptr, nullptr, false, A_END, "");
        return node;
    }
    auto* blockItemNode = blockItem();
    if (blockItemNode == nullptr)
    {
        return nullptr;
    }
    auto* glueNode = new ASTNode;

    //if (tokens->at(cursor).token == CLOSE_BRACE) {printf("Super Hiya!\n"); return glueNode; }
    auto* blockItemListNode = blockItemList();
    if (blockItemListNode == nullptr)
    {
        printf("Oops!\n");
        delete glueNode;
        delete blockItemNode;
        return nullptr;
    }
    ASTNode::fillNode(glueNode, blockItemNode, blockItemListNode, false, A_GLUE, "");
    return glueNode;
}

/* block_item
: declaration
| statement
;
 */
ASTNode* CParse::blockItem() {
    if (isTypeQualifier(tokens->at(cursor)) || isTypeSpecifier(tokens->at(cursor)))
    {
        auto* ctype = new CType;
        bool error = declarationSpecifiers(ctype);
        if (error) return nullptr;
        if (!initDeclaratorList(ctype)) return nullptr;
        auto* emptyNode = new ASTNode;
        cursor++;
        return emptyNode;
    }
    else {
        return statement();
    }
}
/*
 * expression_statement
	: ';'
	| expression ';'
	;
 * */
ASTNode* CParse::expressionStatement() {
    auto* node = new ASTNode;
    if (tokens->at(cursor).token == SEMICOLON) {
        cursor++;
        return node;
    }
    delete node;
    node = expression();
    if (tokens->at(cursor).token != SEMICOLON) {
        delete node;
        return nullptr;
    }
    cursor++;
    return node;
}
/*
 * selection_statement
 	: IF '(' expression ')' statement ELSE statement
 	| IF '(' expression ')' statement
 	;
 * */
ASTNode* CParse::selectionStatement() {
    cursor+=2;
    auto* rootNode = new ASTNode;
    auto* ifCond = expression();
    if (ifCond == nullptr)
    {
        delete rootNode;
        return nullptr;
    }
    if (tokens->at(cursor).token != CLOSE_PARENTHESES)
    {
        delete rootNode;
        delete ifCond;
        print_error(tokens->at(cursor).lineNumber, "Missing ) after expression");
        return nullptr;
    }
    cursor++;
    auto* ifBody = statement();
    if (ifBody == nullptr)
    {
        delete rootNode;
        delete ifCond;
        return nullptr;
    }
    //cursor++;
    if (tokens->at(cursor).token == ELSE)
    {
        cursor++;
        auto* elseNode = statement();
        if (elseNode == nullptr)
        {
            delete rootNode;
            delete ifCond;
            delete ifBody;
            return nullptr;
        }
        auto* ASTGlue = new ASTNode;
        ASTNode::fillNode(rootNode, ifCond, ASTGlue, false, A_IFDECL, "");
        ASTNode::fillNode(ASTGlue, ifBody, elseNode, false, A_IFBODY, "");
        return rootNode;
    }
    auto* ASTGlue = new ASTNode;
    ASTNode::fillNode(rootNode, ifCond, ASTGlue, false, A_IFDECL, "");
    ASTNode::fillNode(ASTGlue, ifBody, nullptr, false, A_IFBODY, "");
    return rootNode;
}
/*
 * iteration_statement
 	: WHILE '(' expression ')' statement
 	| FOR '(' expression_statement expression ')' statement
 	| FOR '(' declaration expression_statement expression ')' statement
 	;
 * */
ASTNode* CParse::iterationStatement() {
    Token token = tokens->at(cursor);
    cursor += 2;
    switch (token.token) {
        case WHILE:
        {
            auto* expr = expression();
            if (expr == nullptr) return nullptr;
            cursor++;
            auto* Statement = statement();
            if (Statement == nullptr)
            {
                delete expr;
                return nullptr;
            }
            auto* rootNode = new ASTNode;
            ASTNode::fillNode(rootNode, expr, Statement, false, A_WHILEBODY, "");
            return rootNode;
        }
        case FOR:
        {
            if (isTypeSpecifier(tokens->at(cursor)) || isTypeQualifier(tokens->at(cursor)))
            {
                auto* scope = new ScopeAST;
                scope->parent = currentScope;
                currentScope = scope;
                auto* type = new CType;
                if (declarationSpecifiers(type)) {
                    ScopeAST* temp = currentScope->parent;
                    delete currentScope;
                    currentScope = temp;
                    return nullptr;
                }
                if (!initDeclaratorList(type)) {
                    print_error("Failure whilst parsing declarator!");
                    ScopeAST* temp = currentScope->parent;
                    delete currentScope;
                    currentScope = temp;
                    delete type;
                    return nullptr;
                }
                cursor++;
            }

            auto* ExpressionStatement = expressionStatement();
            auto* expr = expression();
            cursor++; auto* Statement = statement();
            if (ExpressionStatement == nullptr || expr == nullptr || Statement == nullptr)
            {
                delete expr;
                delete Statement;
                delete ExpressionStatement;
                return nullptr;
            }
            auto* rootNode = new ASTNode;
            auto* forCond = new ASTNode;
            auto* forBody = new ASTNode;
            ASTNode::fillNode(rootNode, nullptr, forCond, false, A_FORDECL, "");
            rootNode->scope = currentScope;
            ASTNode::fillNode(forCond, ExpressionStatement, forBody, false, A_FORCOND, "");
            ASTNode::fillNode(forBody, Statement, expr, false, A_FORBODY, "");
            currentScope = currentScope->parent;
            return rootNode;
        }
        default:
        {
            return nullptr;
        }
    }
    return nullptr;
}
/*
 * jump_statement
 	: RETURN ';'
 	| RETURN expression ';'
 	;*/
ASTNode* CParse::jumpStatement() {
    if (tokens->at(cursor).token != RETURN)
    {
        print_error("Other jump statements than return are not currently supported.");
        return nullptr;
    }
    cursor++;
    if (tokens->at(cursor).token == SEMICOLON)
    {
        auto* node = new ASTNode;
        ASTNode::fillNode(node, nullptr, nullptr, true, A_RET, "");
        cursor++;
        return node;
    }
    auto* node = expression();
    if (node == nullptr) return nullptr;
    auto* rootNode = new ASTNode;
    ASTNode::fillNode(rootNode, node, nullptr, true, A_RET, "");
    return rootNode;
}
/*
 * unary_expression
 	: postfix_expression
 	| INC_OP unary_expression
 	| DEC_OP unary_expression
 	| unary_operator cast_expression
 	| SIZEOF '(' type_name ')'
 	;
 * unary_operator
 	: '&'
 	| '*'
 	| '+'
 	| '-'
 	| '~'
 	| '!'
 	;
 * */
ASTNode* CParse::unaryExpression() {
    switch (tokens->at(cursor).token) {
        case INCREMENT:
        {
            auto* node = new ASTNode;
            cursor++;
            auto* node2 = unaryExpression();
            ASTNode::fillNode(node, node2, nullptr, true, A_INC, "");
            return node;
        }
        case DECREMENT:
        {
            auto* node = new ASTNode;
            cursor++;
            auto* node2 = unaryExpression();
            ASTNode::fillNode(node, node2, nullptr, true, A_DEC, "");
            return node;
        }
        case SIZEOF:
        {
            cursor++;
            if (tokens->at(cursor).token == OPEN_PARENTHESES) {
                auto *node = new ASTNode;
                auto *type = new CType;
                bool suceess = typeName(type);
                if (!suceess) {
                    delete node;
                    delete type;
                    return nullptr;
                }
                int sz = determineSz(type);
                ASTNode::fillNode(node, nullptr, nullptr, false, A_LITERAL, "");
                node->value = sz;
                delete type;
                cursor++;
                return node;
            }
            return nullptr;
        }
        default:
        {
            ASTop op = isUnaryOperator(tokens->at(cursor));
            if (op != A_NOP) {
                cursor++;
                auto* node2 = castExpression();
                if (node2 == nullptr) {return nullptr;}
                auto* node = new ASTNode;
                ASTNode::fillNode(node, node2, nullptr, true, op, "");
                return node;
            }
            return postfixExpression();
        }
    }
}
/*
 * postfix_expression
 	: primary_expression
 	| postfix_expression '(' ')'
 	| postfix_expression '(' argument_expression_list ')'
 	| postfix_expression INC_OP
 	| postfix_expression DEC_OP
 	;*/
ASTNode* CParse::postfixExpression() {
    auto* node = primaryExpression();
    if (node == nullptr) return nullptr;
    if (tokens->at(cursor).token == OPEN_PARENTHESES) {
        auto* rootNode = new ASTNode;
        cursor++;
        if (tokens->at(cursor).token == CLOSE_PARENTHESES)
        {
            ASTNode::fillNode(rootNode, node, nullptr, true, A_CALL, "");
            cursor++;
            return rootNode;
        }
        auto* exprNode = argumentExpressionList();
        if (exprNode == nullptr) {delete node; delete rootNode; return nullptr;}
        ASTNode::fillNode(rootNode, node, exprNode, true, A_CALL, "");
        cursor++;
        return rootNode;
    }
    else if (tokens->at(cursor).token == INCREMENT || tokens->at(cursor).token == DECREMENT)
    {
        auto* rootNode = new ASTNode;
        ASTNode::fillNode(rootNode, node, nullptr, true, tokens->at(cursor).token == INCREMENT ? A_INC : A_DEC, "");
        cursor++;
        return rootNode;
    }
    return node;
}
/*
 * primary_expression
 	: IDENTIFIER
 	| constant
 	| string
 	| '(' expression ')'
 	;*/
ASTNode* CParse::primaryExpression() {
    switch (tokens->at(cursor).token) {
        case IDENTIFIER:
        {
            auto* node = new ASTNode;
            ASTNode::fillNode(node, nullptr, nullptr, true, A_IDENT, tokens->at(cursor).lexeme);
            cursor++;
            return node;
        }
        case INTEGER_LITERAL:
        {
            auto* node = new ASTNode;
            ASTNode::fillNode(node, nullptr, nullptr, false, A_INTLIT, tokens->at(cursor).lexeme);
            cursor++;
            return node;
        }
        case STRING_LITERAL:
        {
            auto* node = new ASTNode;
            const char* null = "\0";
            tokens->at(cursor).lexeme.append(null);
            ASTNode::fillNode(node, nullptr, nullptr, false, A_LITERAL, tokens->at(cursor).lexeme);
            cursor++;
            return node;
        }
        case OPEN_PARENTHESES:
        {
            cursor++;
            auto* node = expression();
            if (tokens->at(cursor).token != CLOSE_PARENTHESES)
            {
                print_error(tokens->at(cursor).lineNumber, "Missing ) after expression");
                delete node;
                return nullptr;
            }
            cursor++;
            return node;
        }
        default:
        {
            return nullptr;
        }
    }
}/*
assignment_expression
 	: conditional_expression
 	| unary_expression assignment_operator assignment_expression
 	;
;
 */
ASTNode* CParse::assignmentExpression() {
    auto* node = conditionalExpression();
    if (node == nullptr)
    {
        return nullptr;
    }
    if (tokens->at(cursor).token == ASSIGNMENT)
    {
        cursor++;
        auto* assignmentExpr = assignmentExpression();
        if (assignmentExpr == nullptr)
            return nullptr;
        auto* assigmentNode = new ASTNode;
        ASTNode::fillNode(assigmentNode, node, assignmentExpr, false, A_MV, "");
        return assigmentNode;
    }
    return node;
}
/*
 * expression
 	: assignment_expression
 	| expression ',' assignment_expression
 	;
*/
ASTNode* CParse::expression() {
    auto* node = assignmentExpression();
    if (node == nullptr) return nullptr;
    if (tokens->at(cursor).token != COMMA)
    {
        return node;
    }
    cursor++;
    auto* glueNode = new ASTNode;
    auto* expr = expression();
    if (expr == nullptr)
    {
        delete node;
        delete glueNode;
        return nullptr;
    }
    ASTNode::fillNode(glueNode, node, expr, false, A_GLUE, "");
    return glueNode;
}
/*
 * statement
 	: labeled_statement --
 	| compound_statement
 	| expression_statement --
 	| selection_statement --
 	| iteration_statement --
 	| jump_statement
 	;
+*/
ASTNode* CParse::statement() {
    if (tokens->at(cursor).token == IDENTIFIER && tokens->at(cursor).token == COLON) {
        return labelStatement();
    }
    else if (tokens->at(cursor).token == IF || tokens->at(cursor).token == SWITCH) {
        return selectionStatement();
    }
    else if (tokens->at(cursor).token == WHILE || tokens->at(cursor).token == FOR || tokens->at(cursor).token == DO) {
        return iterationStatement();
    }
    else if (tokens->at(cursor).token == OPEN_BRACE) {
        return compoundStatement();
    }
    else if (tokens->at(cursor).token == RETURN) {
        return jumpStatement();
    }
    return expressionStatement();
}

ASTNode* CParse::labelStatement() {
    print_error("Label statements are not yet supported by this compiler");
    return nullptr;
}
/*
 * conditional_expression
 	: logical_or_expression
 	| logical_or_expression '?' expression ':' conditional_expression
 	;
*/
ASTNode* CParse::conditionalExpression() {
    auto* node = binaryExpression();
    if (node == nullptr) return nullptr;
    if (tokens->at(cursor).token == QUESTION) {
        auto* rootNode = new ASTNode;
        auto* exprNode = expression();
        cursor++;
        if ((tokens->at(cursor).token != COLON) || (exprNode == nullptr)) {
            delete node;
            delete rootNode;
            delete exprNode;
            return nullptr;
        }
        auto* condNode = binaryExpression();
        if (condNode == nullptr) {
            delete node;
            delete rootNode;
            delete exprNode;
            return nullptr;
        }
        auto* ifBodyNode = new ASTNode;
        ASTNode::fillNode(ifBodyNode, exprNode, condNode, false, A_IFBODY, "");
        ASTNode::fillNode(rootNode, node, ifBodyNode, false, A_IFDECL, "");
        node = rootNode;
    }
    return node;
}


/*
 * cast_expression
 	: unary_expression
 	| '(' type_name ')' cast_expression
 	;*/
ASTNode* CParse::castExpression() {
    if (isTypeSpecifier(tokens->at(cursor+1)))
    {
        ASTNode* node = new ASTNode;
        cursor++;
        auto* rootNode = new ASTNode;
        if (tokens->at(cursor).token != OPEN_PARENTHESES) { delete node; return nullptr; }
        auto* type = new CType;
        bool success = typeName(type);
        if (!success) {delete node; delete type; return nullptr;}
        cursor++;
        auto* node1 = new ASTNode;
        ASTNode::fillNode(node1, nullptr, nullptr, false, A_TYP, "");
        node1->type = type;
        auto* node2 = castExpression();
        ASTNode::fillNode(rootNode, node1, node2, false, A_TYPE_CVT, "");
        node = rootNode;
        return node;
    }
    return unaryExpression();
    
}

std::vector<DeclaratorPieces*>* CParse::abstractDeclarator() {
    auto* declaratorPieces = new std::vector<DeclaratorPieces*>;

    std::vector<Pointer*> pointers = pointer();
    directAbstractDeclarator(declaratorPieces);
    if (declaratorPieces->empty()) {
        for (auto* i : pointers)
        {
            delete i;
        }
        delete declaratorPieces;
        return nullptr;
    }
    declaratorPieces->insert(declaratorPieces->end(), pointers.begin(), pointers.end());

    return declaratorPieces;
}
/*
 * direct_abstract_declarator
	: '(' abstract_declarator ')'
	| '(' ')'
	| '(' parameter_type_list ')'
	| direct_abstract_declarator '(' ')'
	| direct_abstract_declarator '(' parameter_type_list ')'
	;
*/
bool CParse::directAbstractDeclarator(std::vector<DeclaratorPieces *> *declPieces) {
    while (tokens->at(cursor).token != END)
    {
        if (tokens->at(cursor).token == OPEN_PARENTHESES)
        {
            cursor++;
            if (isTypeQualifier(tokens->at(cursor)) || isTypeSpecifier(tokens->at(cursor))) // is some sort of paramList
            {
                auto* x = parameterList();
                declPieces->push_back(x);
            }
            else {
                std::vector<DeclaratorPieces *>* subDeclPieces = abstractDeclarator();
                declPieces->insert(declPieces->begin(), subDeclPieces->begin(), subDeclPieces->end());
                delete subDeclPieces;
            }
            cursor++;
        }
        else {
            //print_error("Unable to parse direct abstract declarator.");
            return false;
        }
        // Now deal with parameter stuff

        while (tokens->at(cursor).token == OPEN_PARENTHESES)
        {
            cursor++;
            auto* funcProto = parameterList();
            declPieces->push_back(funcProto);
            cursor++;
        }
        return true;
    }

    return false;
}

bool CParse::typeName(CType *ctype) {
    bool error = declarationSpecifiers(ctype);
    if (error) {return false;}
    std::vector<DeclaratorPieces*>* x = abstractDeclarator();
    if (x == nullptr) {return false;}
    ctype->declaratorPartList = *x;
    return true;
}
/*
 * argument_expression_list:
 *  assignment_expression
 *  | argument_expression_list ',' assignment_expression
 * */
ASTNode* CParse::argumentExpressionList() {
    auto* node = assignmentExpression();
    if (node == nullptr)
        return nullptr;
    if (tokens->at(cursor).token != COMMA)
    {
        return node;
    }
    cursor++;
    auto* glueNode = new ASTNode;
    auto* expr = argumentExpressionList();
    if (expr == nullptr)
    {
        delete glueNode;
        delete node;
        return nullptr;
    }
    ASTNode::fillNode(glueNode, node, expr, false, A_GLUE, "");
    return glueNode;
}

ASTop isBinOp(Token& token)
{
    auto value = binHashMap.find(token.token);
    if (value == binHashMap.end())
    {
        return A_NOP;
    }
    return value->second;

}
/* from pycparser
 * binary_expression   : cast_expression
                                | binary_expression TIMES binary_expression
                                | binary_expression DIVIDE binary_expression
                                | binary_expression MOD binary_expression
                                | binary_expression PLUS binary_expression
                                | binary_expression MINUS binary_expression
                                | binary_expression RSHIFT binary_expression
                                | binary_expression LSHIFT binary_expression
                                | binary_expression LT binary_expression
                                | binary_expression LE binary_expression
                                | binary_expression GE binary_expression
                                | binary_expression GT binary_expression
                                | binary_expression EQ binary_expression
                                | binary_expression NE binary_expression
                                | binary_expression AND binary_expression
                                | binary_expression OR binary_expression
                                | binary_expression XOR binary_expression
                                | binary_expression LAND binary_expression
                                | binary_expression LOR binary_expression
*/
ASTNode* CParse::binaryExpression() {
    auto* node = castExpression();
    if (node == nullptr) return nullptr;
    while (true)
    {
        ASTop op = isBinOp(tokens->at(cursor));
        if (op == A_NOP) {
            return node;
        }
        cursor++;
        auto* secNode = binaryExpression();
        if (secNode == nullptr) {
            delete node;
            return nullptr;
        }
        auto* rootNode = new ASTNode;
        ASTNode::fillNode(rootNode, node, secNode, false, op, "");
        node = rootNode;
    }
}
/*
 * constant_expression:
 *      numerical_literal;
 * */
ASTNode *CParse::constantExpression() {
    if (tokens->at(cursor).token != INTEGER_LITERAL)
    {
        return nullptr;
    }
    auto* node = new ASTNode;
    ASTNode::fillNode(node, nullptr, nullptr, false, A_INTLIT, "");
    node->value = std::stoi(tokens->at(cursor).lexeme);
    cursor++;
    return node;
}



bool CParse::initDeclaratorList(CType *ctype) {
    Symbol* symbol = initDeclarator(ctype);
    if (symbol == nullptr) return false;
    currentScope->rst.SymbolHashMap.insert({symbol->identifier, globalIndex});
    globalSymbolTable.push_back(symbol); // add declarator to symbol table
    globalIndex++;

    identifier = symbol->identifier;

    return true;
}

Symbol* CParse::initDeclarator(CType *ctype) {
    auto* symbol = new Symbol;

    std::vector<DeclaratorPieces*>* y = declarator();
    if (y == nullptr)
    {
        delete symbol;
        return nullptr;
    }
    ctype->declaratorPartList = *y;
    delete y;
    symbol->type = ctype;
    if (ctype->declaratorPartList.empty()) return nullptr;
    if (ctype->declaratorPartList.at(0)->getDPT() != D_IDENTIFIER )
    {
        delete symbol;
        return nullptr;
    }
    auto* x = dynamic_cast<Identifier*>(ctype->declaratorPartList[0]);
    symbol->identifier = x->identifier_name;

    if (tokens->at(cursor).token == ASSIGNMENT && tokens->at(cursor+1).token == INTEGER_LITERAL)
    {
        symbol->value = std::stoi(tokens->at(cursor+1).lexeme); // store initial value
        cursor+=2;
    }

    return symbol;
}
std::vector<DeclaratorPieces*>* CParse::declarator() {
    auto* declaratorPieces = new std::vector<DeclaratorPieces*>;

    std::vector<Pointer*> pointers = pointer();
    directDeclarator(declaratorPieces);
    if (declaratorPieces->empty()) {
        for (auto* i : pointers)
        {
            delete i;
        }
        delete declaratorPieces;
        return nullptr;
    }
    declaratorPieces->insert(declaratorPieces->end(), pointers.begin(), pointers.end());

    return declaratorPieces;
}

std::vector<Pointer*> CParse::pointer() {
    std::vector<Pointer*> pointers;
    while (tokens->at(cursor).token == STAR) {
        auto* nPointer = new Pointer;
        cursor++;
        while(isTypeQualifier(tokens->at(cursor)))
        {
            if (tokens->at(cursor).token == VOLATILE) nPointer->setVolatile();
            if (tokens->at(cursor).token == CONST) nPointer->setConst();
            cursor++;
        }
        pointers.push_back(nPointer);
    }
    return pointers;
}
/*
 * direct_declarator
	: IDENTIFIER
	| '(' declarator ')'
	| direct_declarator '(' parameter_type_list ')'
	| direct_declarator '(' ')'
	;*/
bool CParse::directDeclarator(std::vector<DeclaratorPieces *> *declPieces) {
    while (tokens->at(cursor).token != END)
    {
        if (tokens->at(cursor).token == IDENTIFIER)
        {

            auto* identifierx = new Identifier;
            identifierx->identifier_name = tokens->at(cursor).lexeme;
            declPieces->insert(declPieces->begin(), identifierx);
            cursor++;
        }
        else if (tokens->at(cursor).token == OPEN_PARENTHESES)
        {
            cursor++;
            std::vector<DeclaratorPieces *>* subDeclPieces = declarator();
            if (subDeclPieces == nullptr)
            {
                return false;
            }
            declPieces->insert(declPieces->begin(), subDeclPieces->begin(), subDeclPieces->end());
            delete subDeclPieces;
            cursor++;
        }
        else {
            //print_error(tokens->at(cursor).lineNumber, "Unable to parse direct declarator.");
            return false;
        }
        // Now deal with parameter stuff

        while (tokens->at(cursor).token == OPEN_PARENTHESES)
        {
            cursor++;
            if (tokens->at(cursor).token == CLOSE_PARENTHESES)
            {
                auto* funcProto = new FunctionPrototype;
                auto* scope = new ScopeAST;
                scope->parent = currentScope;
                funcProto->scope = scope;
                declPieces->push_back(funcProto);
                cursor++;
                return true;
            } else {
                auto* funcProto = parameterList();
                declPieces->push_back(funcProto);
            }
        }
        return true;
    }

    return false;
}

/*
parameter_list
	: parameter_declaration
	| parameter_list ',' parameter_declaration
	;
*/
FunctionPrototype* CParse::parameterList() {
    auto* funcProto = new FunctionPrototype;
    auto* scopeAST = new ScopeAST;
    scopeAST->parent = currentScope;
    currentScope = scopeAST;
    bool end = false;
    while (!end)
    {
        auto* paramDecl = parameterDecleration();
        if (paramDecl == nullptr)
        {
            delete funcProto;
            return nullptr;
        }
        funcProto->types.push_back(paramDecl);
        if (!paramDecl->abstractdecl) { // add to symbol table if not an abstract declarator.
            if (scopeAST->findSymbolInLocalScope(paramDecl->identifier) != -1) { // found
                delete funcProto;
                return nullptr;
            }
            globalSymbolTable.push_back(paramDecl);
            scopeAST->rst.SymbolHashMap[paramDecl->identifier] = globalIndex;
            globalIndex++;
        } else {
            globalSymbolTable.push_back(paramDecl);
            globalIndex++;
        }

        if (tokens->at(cursor).token != COMMA)
        {
            end = true;
        }
        cursor++;
    }
    currentScope = currentScope->parent;
    funcProto->scope = scopeAST;
    return funcProto;
}

Symbol* CParse::parameterDecleration() {
    auto* ctype = new CType;

    bool error = declarationSpecifiers(ctype);
    if (error)
    {
        delete ctype;
        return nullptr;
    }

    std::vector<DeclaratorPieces*>* dp = declarator();
    if (dp == nullptr)
    {
        dp = abstractDeclarator();
    }
    if (dp == nullptr)
    {
        auto* symbol = new Symbol;
        symbol->type = ctype;
        symbol->abstractdecl = true;
        return symbol;
    }
    ctype->declaratorPartList.insert(ctype->declaratorPartList.begin(), dp->begin(), dp->end());
    if (dp->empty())
    {
        delete dp;
        return nullptr;
    }
    if (dp->at(0)->getDPT() == D_IDENTIFIER) // Not abstract, add to symbol table
    {
        auto* ident = dynamic_cast<Identifier*>(dp->at(0));
        auto* symbol = new Symbol;
        symbol->type = ctype;
        symbol->identifier = ident->identifier_name;
        symbol->abstractdecl = false;
        delete dp;
        return symbol;
    }
    else {
        auto* symbol = new Symbol;
        symbol->type = ctype;
        symbol->identifier = "";
        symbol->abstractdecl = true;
        delete dp;
        return symbol;
    }
}

/*
compound_statement
	: '{' '}'
    | '{' block_item_list '}'
	;
assignment_expression
	: conditional_expression
	| unary_expression assignment_operator assignment_expression
	;


 * */


i64 ScopeAST::findRegularSymbol(const std::string &identifier) {
    auto value = rst.SymbolHashMap.find(identifier);
    if (value != rst.SymbolHashMap.end())
    {
        return value->second;
    }
    else {
        if (parent != nullptr)
            return parent->findRegularSymbol(identifier);
    }
    return -1;
}

i64 ScopeAST::findSymbolInLocalScope(const std::string &identifier) {
    auto value = rst.SymbolHashMap.find(identifier);
    if (value != rst.SymbolHashMap.end())
    {
        return value->second;
    }

    return -1;
}

i64 ScopeAST::findEarliestScopeLevel(i64 startVal, const std::string &identifier) {
    auto value = rst.SymbolHashMap.find(identifier);
    if (value != rst.SymbolHashMap.end())
    {
        return startVal;
    }
    else {
        if (parent != nullptr)
            return parent->findEarliestScopeLevel(startVal+1, identifier);
    }
    return -1;
}



FunctionPrototype::~FunctionPrototype() {
    if (!doNotDeleteScope)
        delete scope;
}

std::string FunctionPrototype::print() {
    std::string temp;
    for (auto* i: types) {
        temp.append(i->type->typeAsString());
        temp.append(",");
    }
    return temp;
}

ASTNode::~ASTNode() {

}

void ASTNode::deleteNode(ASTNode *node) {
    if (node == nullptr)
        return;
    deleteNode(node->left);
    deleteNode(node->right);
    if (!node->doNotDeleteType)
        delete node->type;
    delete node->scope;
    delete node;
}

bool CType::isCompatible(CType type, ASTop op) {
    return false;
}

bool CType::isPtr() {
    i32 cursor = 0;
    if (declaratorPartList.empty())
        return false;
    if (declaratorPartList.at(0)->getDPT() == D_IDENTIFIER)
    {
        cursor = 1;
        if (declaratorPartList.size() == 1)
            return false;
    }

    if (declaratorPartList.at(cursor)->getDPT() == PTR)
    {
        return true;
    }
    return false;
}

bool CType::isArray() {
    i32 cursor = 0;
    if (declaratorPartList.empty())
        return false;
    if (declaratorPartList.at(0)->getDPT() == D_IDENTIFIER)
    {
        cursor = 1;
        if (declaratorPartList.size() == 1)
            return false;
    }
    if (declaratorPartList.at(cursor)->getDPT() == ARR)
    {
        return true;
    }
    return false;
}

bool CType::isNumVar() {
    if (declaratorPartList.empty())
        return true;

    if (declaratorPartList.at(0)->getDPT() == D_IDENTIFIER)
    {
        if (declaratorPartList.size() == 1)
            return true;
        return false;
    }
    return false;
}

bool CType::isFuncPtr() {
    i32 cursor = 0;
    if (declaratorPartList.empty())
        return false;
    if (declaratorPartList.at(0)->getDPT() == D_IDENTIFIER)
    {
        cursor = 1;
        if (declaratorPartList.size() == 1)
            return false;
    }

    if (declaratorPartList.at(cursor)->getDPT() == PTR)
    {
        if (declaratorPartList.size() == cursor+1)
            return false;

        if (declaratorPartList.at(cursor+1)->getDPT() == FUNC)
        {
            return true;
        }
    }
    return false;
}

bool CType::isEqual(CType *otherType, bool ptrOrNum) {
    if (otherType == nullptr) return false;
    bool notEqual = false;
    Token token;
    Token token2;
    for (const auto& i: typeSpecifier)
    {
        if (isTypeSpecifier(i))
            token.token = i.token;
    }
    for (const auto& i : otherType->typeSpecifier)
    {
        if (isTypeSpecifier(i))
            token2.token = i.token;
    }

    if (token.token == token2.token && ptrOrNum)
        return true;
    if (token.token != token2.token && ptrOrNum) {
        return true;
    }

    if (token.token != token2.token && !ptrOrNum) {
        return false;
    }

    //if (otherType->declaratorPartList.size() != declaratorPartList.size())
    //    return false;



    bool equal = true;
    int x = otherType->declaratorPartList.at(0)->getDPT() == D_IDENTIFIER ? 1 : 0;
    int y = declaratorPartList.at(0)->getDPT() == D_IDENTIFIER ? 1 : 0;
    while (equal && (y < declaratorPartList.size()) && (x < otherType->declaratorPartList.size()))
    {
        equal = (declaratorPartList.at(y)->getDPT() == otherType->declaratorPartList.at(x)->getDPT());
        x++;
        y++;
    }
    return equal;
}

std::string CType::typeAsString() {
    std::string type{};
    for (const auto& i: typeSpecifier)
    {
        type.append(i.lexeme);
    }

    return type;
}

CType* CType::dereferenceType() {
    if (!isPtr())
        return nullptr;
    auto* ctype = new CType;
    int cursor = 0;
    if (declaratorPartList.at(cursor)->getDPT() == D_IDENTIFIER)
    {
        cursor = 1;
    }
    copy(ctype);
    auto* ptr = dynamic_cast<Pointer*>(ctype->declaratorPartList.at(cursor));
    ctype->declaratorPartList.erase(ctype->declaratorPartList.cbegin()+cursor);
    delete ptr;
    return ctype;
}

CType *CType::refType() {
    if (typeSpecifier.at(0).token == VOID)
    {
        return nullptr;
    }
    auto* ctype = new CType;
    int cursor = 0;
    if (declaratorPartList.at(cursor)->getDPT() == D_IDENTIFIER)
    {
        cursor = 1;
    }
    copy(ctype);
    auto* pointer = new Pointer;
    ctype->declaratorPartList.insert(ctype->declaratorPartList.cbegin()+cursor, pointer);
    return ctype;
}

void CType::copy(CType* x) {
    x->typeSpecifier = typeSpecifier;
    for (auto* i : declaratorPartList)
    {
        switch (i->getDPT()) {
            case PTR: {
                auto* pointer = new Pointer;
                Pointer pointer2 = *(dynamic_cast<Pointer*>(i));
                if (pointer2.isConstPtr())
                {
                    pointer->setConst();
                }
                if (pointer2.isVolatilePtr())
                {
                    pointer->setVolatile();
                }
                x->declaratorPartList.push_back(pointer);
                break;
            }
            case FUNC: {
                auto* funcProto2 = new FunctionPrototype;
                FunctionPrototype funcProto = *(dynamic_cast<FunctionPrototype*>(i));
                funcProto2->scope = funcProto.scope;
                funcProto2->types = funcProto.types;
                funcProto2->doNotDeleteScope = true;
                x->declaratorPartList.push_back(funcProto2);
                break;
            }
            case ARR: {
                break;
            }
            case D_IDENTIFIER: {
                Identifier identifier = *(dynamic_cast<Identifier*>(i));
                auto* newIdentifier =  new Identifier;
                newIdentifier->identifier_name = identifier.identifier_name;
                x->declaratorPartList.push_back(newIdentifier);
                break;
            }
        }
    }
}

bool CType::isStatic() {
    bool found = false;
    for (const auto& i : typeSpecifier) {
        if (i.token == STATIC)
            return true;
    }
}

std::string DeclaratorPieces::print() {
    return std::string();
}

AVMFunction::~AVMFunction() {
    for (auto i : basicBlocksInFunction)
        delete i;
}

AVMBasicBlock::~AVMBasicBlock() {
    for (auto i : sequenceOfInstructions)
        delete i;
}
