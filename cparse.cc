#include "cparse.hh"
#include <errorHandler.hh>
// Recursive Descent Parser
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
    if ((token.token == INTEGER) || (token.token == CHAR) || (token.token == SHORT))
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
 */
bool CParse::parse()
{
    auto* globalScope = new ScopeAST;
    currentScope = globalScope;
    currentScope->parent = nullptr;
    while (tokens->at(cursor).token != END)
    {
        auto* type = new CType;
        if (declarationSpecifiers(type)) return false;

        if (!initDeclaratorList(type)) {
            print_error("Failure whilst parsing declarator!");
            return false;
        }
        if (tokens->at(cursor).token != SEMICOLON)
        {
            if (tokens->at(cursor).token == OPEN_BRACE) {
                cursor++;
                auto* function = new FunctionAST(type);

                complexStatement();
            }
            print_error("Expected semicolon!");
            return false;
        }
        cursor++;
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

CParse::~CParse()
{

}
/*
 * init_declarator_list
	: init_declarator
	| init_declarator_list ',' init_declarator
	;
*/
bool CParse::initDeclaratorList(CType *ctype) {
    bool finished = false;
    while (tokens->at(cursor).token != END && !finished) {
        while (!finished)
        {
            Symbol* symbol = initDeclarator(ctype);
            if (tokens->at(cursor).token != COMMA)
                finished = true;
            if (!symbol) return false;
            currentScope->rst.SymbolHashMap[symbol->identifier] = currentScope->rst.id;
            currentScope->rst.symbols.push_back(*symbol);
            currentScope->rst.id++;
        }
    }
    return tokens->at(cursor).token != END;
}
/*
 * init_declarator
	: declarator
	| declarator '=' initializer
	;
    initlializer: numeric_lit;
 * */
Symbol* CParse::initDeclarator(CType* ctype)
{
    auto* symbol = new Symbol;

    ctype->declaratorPartList = *declarator();
    symbol->type = ctype;
    if (ctype->declaratorPartList.empty()) return nullptr;
    if (ctype->declaratorPartList.at(0)->getDPT() != D_IDENTIFIER )
        return nullptr;
    auto* x = dynamic_cast<Identifier*>(ctype->declaratorPartList[0]);
    symbol->identifier = x->identifier_name;
    if (tokens->at(cursor).token == EQUAL && tokens->at(cursor++).token == INTEGER_LITERAL)
    {
        symbol->value = std::stoi(tokens->at(cursor++).lexeme);
    }

    return symbol;
}
/*
 * declarator
	: pointer direct_declarator
	| direct_declarator
	;
 * */
std::vector<DeclaratorPieces*>* CParse::declarator() {
    auto* declaratorPieces = new std::vector<DeclaratorPieces*>;

    std::vector<Pointer*> pointers = pointer();
    directDeclarator(declaratorPieces);
    if (declaratorPieces->empty()) print_error("WARN: decl part list empty");
    declaratorPieces->insert(declaratorPieces->end(), pointers.begin(), pointers.end());

    return declaratorPieces;
}
/*pointer
	: '*'
	| '*' type_qualifier_list
	| '*' pointer
	| '*' type_qualifier_list pointer
	;*/
std::vector<Pointer*> CParse::pointer()
{
    std::vector<Pointer*> pointers;
    while (tokens->at(cursor).token == STAR) {
        auto* nPointer = new Pointer;
        cursor++;
        while(!isTypeQualifier(tokens->at(cursor)))
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
	;
 abstract_declarator
	: pointer
	| direct_abstract_declarator
	| pointer direct_abstract_declarator
	;

direct_abstract_declarator
	: '(' abstract_declarator ')'
	| '(' ')'
	| '(' parameter_type_list ')'
	| direct_abstract_declarator '(' ')'
	| direct_abstract_declarator '(' parameter_type_list ')'
	;
 */
bool CParse::directDeclarator(std::vector<DeclaratorPieces*>* declPieces) {
    while (tokens->at(cursor).token != END)
    {
        printf("%d", cursor);
        if (tokens->at(cursor).token == IDENTIFIER)
        {

            printf("Hello");
            auto* identifier = new Identifier;
            identifier->identifier_name = tokens->at(cursor).lexeme;
            declPieces->insert(declPieces->begin(), identifier);
            cursor++;
        }
        else if (tokens->at(cursor).token == OPEN_PARENTHESES)
        {
            cursor++;
            if (isTypeQualifier(tokens->at(cursor)) || isTypeSpecifier(tokens->at(cursor))) // is some sort of paramList
            {
                auto* x = parameterList();
                declPieces->push_back(x);
            }
            else {
                std::vector<DeclaratorPieces *>* subDeclPieces = declarator();
                declPieces->insert(declPieces->begin(), subDeclPieces->begin(), subDeclPieces->end());
            }
            cursor++;
        }
        else {
            print_error("Unable to parse direct declarator.");
            return false;
        }
        // Now deal with parameter stuff

        if (tokens->at(cursor).token == OPEN_PARENTHESES) // allow abstract declarators to use this as well
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
/*
parameter_list
	: parameter_declaration
	| parameter_list ',' parameter_declaration
	;

*/
FunctionPrototype* CParse::parameterList()
{
    auto* scopedSymbols = new ScopeAST;
    scopedSymbols->parent = currentScope;
    auto* funcProto = new FunctionPrototype;
    bool error = false;
    bool done = false;
    while (!done && !error)
    {
        auto* x = parameterDecleration();
        if (!x) return nullptr;
        if (x->abstractdecl)
        {
            funcProto->types.push_back(x->type);
        }
        else {
            if (!currentScope->findRegularSymbol(x->identifier))
            {
                print_error("Redecleration of already declared identifier in scope, whilst parsing parameter list.");
                done = true;
                error = true;
            }
            else {
                scopedSymbols->rst.SymbolHashMap[x->identifier] = scopedSymbols->rst.id;
                scopedSymbols->rst.symbols.push_back(*x);
                scopedSymbols->rst.id++;
                funcProto->types.push_back(x->type);
            }
        }
        if (tokens->at(cursor).token != COMMA)
        {
            done = true;
        }
    }
    funcProto->scope = scopedSymbols;
    currentScope = scopedSymbols->parent;
    return funcProto;
}
/*
 * parameter_declaration
	: declaration_specifiers declarator
	| declaration_specifiers abstract_declarator
	| declaration_specifiers
	;
 */
Symbol* CParse::parameterDecleration() {
    auto* ctype = new CType;

    bool success = declarationSpecifiers(ctype);
    if (!success)
    {
        return nullptr;
    }
    std::vector<DeclaratorPieces*>* dp = declarator();
    ctype->declaratorPartList.insert(ctype->declaratorPartList.begin(), dp->begin(), dp->end());
    if (dp->empty())
    {
        return nullptr;
    }
    if (dp->at(0)->getDPT() == D_IDENTIFIER) // Not abstract, add to symbol table
    {
        auto* ident = dynamic_cast<Identifier*>(dp->at(0));
        auto* symbol = new Symbol;
        ctype->declaratorPartList.erase(ctype->declaratorPartList.begin());
        symbol->type = ctype;
        symbol->identifier = ident->identifier_name;
        symbol->abstractdecl = false;
        return symbol;
    }
    // Abstract
    auto* symbol = new Symbol;
    symbol->type = ctype;
    symbol->abstractdecl = true;
    return symbol;
}


bool CParse::combinable(CType *cType, const Token& token)
{
    switch (token.token) {
        case SHORT:
        case CHAR:
        case INTEGER: {
            bool conflictingTypeFound = false;
            for (auto& i: cType->typeSpecifier)
            {
                if (i.token == CHAR || i.token == SHORT || i.token == INTEGER) // Add floating point types as well
                    conflictingTypeFound = true;
            }
            if (conflictingTypeFound) {
                return false;
            }
            else {return true;}
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
	| '{' statement_list '}'
	| '{' declaration_list '}'
	| '{' declaration_list statement_list '}'
	;
assignment_expression
	: conditional_expression
	| unary_expression assignment_operator assignment_expression
	;

 * */
void CParse::complexStatement() {
    if (tokens->at(cursor).token == IDENTIFIER) // x = 2 + 3 + 5;
    {
        auto* symbol = currentScope->findRegularSymbol(tokens->at(cursor).lexeme);
        cursor++;
        auto* node = infixExpression();

    }
}

ASTNode* CParse::infixExpression() //For now only supports two constants to be added
{
    auto* node = new ASTNode;

    node->op = A_ADD;
    node->left = new ASTNode;
    node->left->op = A_INTLIT;
    node->left->value = std::stoi(tokens->at(cursor).lexeme);
    cursor+=2;
    node->left->unary = true;
    node->right = new ASTNode;
    node->right->op = A_INTLIT;
    node->right->value = std::stoi(tokens->at(cursor).lexeme);
    cursor+=2;
    node->right->unary = true;
    return node;
}


Symbol* ScopeAST::findRegularSymbol(const std::string &identifier) {
    auto value = rst.SymbolHashMap.find(identifier);
    if (value != rst.SymbolHashMap.end())
    {
        return &rst.symbols.at(value->second);
    }
    else if (parent != nullptr)
    {
        findRegularSymbol(identifier);
    }
    return nullptr;
}

FunctionAST::FunctionAST(CType *declaratorType, ScopeAST* parent) {
    auto* symbol = parent->findRegularSymbol(identifier);
    if (symbol)
    {

    }
}
