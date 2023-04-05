#pragma once
#include <sfce.hh>
#include <lexer.hh>
#include <memory>
enum ASTop {
    A_ADD,
    A_SUB,
    A_DIV,
    A_NOP,
    A_MULT,
    A_SLL,
    A_ASR,
    A_MODULO,
    A_NEQ,
    A_EQ,
    A_AND,
    A_OR,
    A_XOR,
    A_CALL,
    A_TYPE_CVT,
    A_INTLIT,
    A_IDENT,
    A_RET,
    A_IFBODY,
    A_IFDECL,
    A_WHILEBODY,
    A_LITERAL,
    A_DEREF,
    A_AGEN,
    A_GLUE,
    A_LOR,
    A_LAND,
    A_LT,
    A_MT,
    A_MTEQ,
    A_LTEQ,
    A_TYP,
    A_INC,
    A_DEC,
    A_ABS,
    A_NEG,
    A_NOT,
    A_LNOT,
    A_FORCOND,
    A_FORBODY,
    A_FORDECL,
    A_CS,
    A_MV,
    A_END
};
enum DeclaratorPieceType
{
    PTR,
    FUNC,
    ARR,
    D_IDENTIFIER
};

class DeclaratorPieces
{
public:
    virtual DeclaratorPieceType getDPT() {return dpt;};

protected:
    DeclaratorPieceType dpt = PTR;

};
class Pointer : public DeclaratorPieces
{
public:
    DeclaratorPieceType getDPT() final {return dpt;};
    void setConst() {isConst = true;};
    void setVolatile() {isVolatile = true;};
    bool isConstPtr() {return isConst;};
    bool isVolatilePtr() {return isVolatile;};
private:
    DeclaratorPieceType dpt = PTR;
    bool isConst = false;
    bool isVolatile = false;
};
class ScopeAST;
struct CType
{
    ~CType() {
        for (auto* i : declaratorPartList)
        {
            delete i;
        }
    };
    std::vector<Token> typeSpecifier;
    std::vector<DeclaratorPieces*> declaratorPartList;
    char bitfield = 0; // Used for unions, currently unsupported **TODO**

    bool isCompatible(CType type, ASTop op);
    bool isPtr();
    bool isArray();
    bool isNumVar();
    bool isFuncPtr();

private:
    bool assignCompat(CType type);
};
class FunctionPrototype : public DeclaratorPieces
{
public:
    ~FunctionPrototype();
    DeclaratorPieceType getDPT() final {return dpt;};
    ScopeAST* scope = nullptr;

    std::vector<CType> types{};
private:
    DeclaratorPieceType dpt = FUNC;
};

class Array : public DeclaratorPieces
{
public:
    DeclaratorPieceType getDPT() final {return dpt;};
    void setArraysz(u64 size) {arraySz = size;};
    u64 getSize() {return arraySz;};
private:
    u64 arraySz = 0;
    DeclaratorPieceType dpt = ARR;
};

class Identifier : public DeclaratorPieces
{
public:
    DeclaratorPieceType getDPT() final {return dpt;};
    std::string identifier_name;
private:
    DeclaratorPieceType dpt = D_IDENTIFIER;
};


struct Symbol {
    ~Symbol() = default;
    u64 value = 0;
    CType type;
    bool abstractdecl = false;
    std::string identifier;
};


class ASTNode
{
public:
    ASTNode() = default;
    ~ASTNode();
    ASTNode* left = nullptr; // If Expression is unary it is always to the left
    ASTNode* right = nullptr;
    bool unary = false;
    enum ASTop op = A_NOP;
    std::string identifier;
    CType type; // only usable if op = A_TYPE_CVT
    ScopeAST* scope = nullptr;
    static void print(ASTNode* node) {
        if (node == nullptr)
            return;
        print(node->left);
        print(node->right);
        printf("OP: %d value: %lu, identifier: %s\n", node->op, node->value, node->identifier.c_str());
    }
    static void deleteNode(ASTNode* node);
    static void fillNode(ASTNode* node, ASTNode* left, ASTNode* right, bool unary, ASTop op, const std::string& identifier) {
        node->left = left;
        node->right = right;
        node->op = op;
        node->unary = unary;
        node->identifier = identifier;
    }
    u64 value = 0;
};
struct RegularSymbolTable
{
    u32 id = 0;
    std::vector<Symbol> symbols;
    std::unordered_map<std::string, u32> SymbolHashMap; // Map function/prototypes to ids in an array
};

struct StructMemberSymbolTable
{
    u32 idx = 0;
    std::vector<Symbol> symbols;
    std::unordered_map<std::string, u32> SymbolHashMap; // Map function/prototypes to ids in an array
};
struct StructSymbolTable
{
    u32 idx = 0;
    std::vector<Symbol> symbols;
    std::unordered_map<std::string, u32> SymbolHashMap; // Map function/prototypes to ids in an array
    StructMemberSymbolTable memberTable;
};
struct LabelSymbolTable
{
    u32 idx = 0;
    std::vector<Symbol> symbols;
    std::unordered_map<std::string, u32> SymbolHashMap; // Map function/prototypes to ids in an array
};
struct ScopeAST {
    ScopeAST* parent = nullptr;
    ~ScopeAST() = default;
    RegularSymbolTable rst;
    StructSymbolTable sst;
    LabelSymbolTable lst;

    static Symbol* findRegularSymbol(ScopeAST* scope, const std::string& identifier);
    Symbol* findSymbolInLocalScope(const std::string& identifier);
    Symbol* findStructSymbol(const std::string& identifier);
    Symbol* findLabelSymbol(const std::string& identifier);
};

class FunctionAST
{
public:
    explicit FunctionAST(CType& declaratorType, const std::string& identifier)
    {
        returnType.typeSpecifier = std::move(declaratorType.typeSpecifier);
        int cursor = 0;
        funcIdentifier = dynamic_cast<Identifier*>(declaratorType.declaratorPartList.at(cursor))->identifier_name;
        cursor++;
        while (declaratorType.declaratorPartList.at(cursor)->getDPT() == PTR) {
            returnType.declaratorPartList.push_back(declaratorType.declaratorPartList.at(cursor));
            cursor++;
        }
        if (declaratorType.declaratorPartList.at(cursor)->getDPT() == FUNC)
        {
            printf("Hiya\n");
            prototype = *dynamic_cast<FunctionPrototype*>(declaratorType.declaratorPartList.at(cursor));
        }

    }; // append parameter list from FunctionParameter type + extract return type
    ~FunctionAST() = default;
    ASTNode* root = nullptr;
    void printFunction() const {
        ASTNode::print(root);
    }
    bool funcDefinedInFile = false;
    bool funcDeclInFile = false;
    std::string funcIdentifier;
    CType returnType;
    FunctionPrototype prototype{};
};

class CParse
{
public:
    explicit CParse(std::vector<Token>* input);
    bool parse();
    ~CParse();
private:
    std::vector<Token>* tokens;
    u32 cursor = 0;
    ScopeAST* currentScope = nullptr;
    std::vector<FunctionAST*> functions;
    FunctionAST* currentFunction{};

    std::vector<DeclaratorPieces*>* declarator();
    bool declarationSpecifiers(CType* cType);
    static bool combinable(CType* cType, const Token& token);
    bool initDeclaratorList(CType* ctype);
    Symbol* initDeclarator(CType* ctype);
    std::vector<Pointer*> pointer();
    std::vector<DeclaratorPieces *> * abstractDeclarator();
    bool directDeclarator(std::vector<DeclaratorPieces*>* declPieces);
    bool directAbstractDeclarator(std::vector<DeclaratorPieces*>* declPieces);
    FunctionPrototype* parameterList();
    Symbol* parameterDecleration();

    ASTNode* blockItemList();
    ASTNode* blockItem();
    ASTNode* compoundStatement();
    ASTNode* expressionStatement();
    ASTNode* selectionStatement();
    ASTNode* iterationStatement();
    ASTNode* jumpStatement();
    ASTNode* unaryExpression();
    ASTNode* postfixExpression();
    ASTNode* primaryExpression();
    ASTNode* conditionalExpression();
    ASTNode* assignmentExpression();
    ASTNode* labelStatement();
    ASTNode* expression();
    ASTNode* statement();
    ASTNode* constantExpression();
    ASTNode* castExpression();
    ASTNode* argumentExpressionList();
    ASTNode* binaryExpression();
    bool typeName(CType* ctype);

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

    ASTop isBinOp(Token& token);
    std::string identifier;
    bool fuse = false;
};