#pragma once
#include <sfce.hh>
#include <lexer.hh>
#include <memory>
enum ASTop {
    A_ADD,
    A_SUB,
    A_DIV,
    A_MULT,
    A_SLL,
    A_ASR,
    A_MODULO,
    A_BLTEQ,
    A_BLT,
    A_BMT,
    A_BMTEQ,
    A_BEQ,
    A_BNEQ,
    A_AND,
    A_OR,
    A_XOR,
    A_CALL,
    A_TYPE_CVT,
    A_MV,
    A_INTLIT,
    A_IDENT,
    A_RET,
    A_IFBODY,
    A_IFDECL,
    A_WHILEDECL,
    A_WHILEBODY,
    A_LITERAL,
    A_DEREF,
    A_AGEN,
    A_SCALE
};
enum DeclaratorPieceType
{
    PTR,
    FUNC,
    ARR,
    D_IDENTIFIER
};
struct CType;
struct Symbol {
    u64 value = 0;
    CType* type = nullptr;
    bool abstractdecl = false;
    std::string identifier;
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

class FunctionPrototype : public DeclaratorPieces
{
public:
    DeclaratorPieceType getDPT() final {return dpt;};
    std::vector<Symbol> Parameters;
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

struct CType
{
    std::vector<Token> typeSpecifier;
    std::vector<DeclaratorPieces> declaratorPartList;
    char bitfield; // Used for unions, currently unsupported **TODO**

    bool isCompatible(CType type, ASTop op);
    bool isPtr();
    bool isArray();
    bool isNumVar();
    bool isFuncPtr();
private:
    bool assignCompat(CType type);
};

class ASTNode
{
public:
    ASTNode();
    ~ASTNode();
    ASTNode* left; // If Expression is unary it is always to the left
    ASTNode* right;
    bool unary;
    enum ASTop op;
    std::string identifier;
};
struct RegularSymbolTable
{
    i32 idx;
    std::vector<Symbol> symbols;
    std::unordered_map<std::string, int> SymbolHashMap; // Map function/prototypes to ids in an array
};

struct StructMemberSymbolTable
{
    i32 idx;
    std::vector<Symbol> symbols;
    std::unordered_map<std::string, int> SymbolHashMap; // Map function/prototypes to ids in an array
};
struct StructSymbolTable
{
    i32 idx;
    std::vector<Symbol> symbols;
    std::unordered_map<std::string, int> SymbolHashMap; // Map function/prototypes to ids in an array
    StructMemberSymbolTable memberTable;
};
struct LabelSymbolTable
{
    i32 idx;
    std::vector<Symbol> symbols;
    std::unordered_map<std::string, int> SymbolHashMap; // Map function/prototypes to ids in an array
};
struct ScopeAST {
    ScopeAST* parent = nullptr;
    RegularSymbolTable rst;
    StructSymbolTable sst;
    LabelSymbolTable lst;

    i32 findRegularSymbol(const std::string& identifier);
    i32 findStructSymbol(const std::string& identifier);
    i32 findLabelSymbol(const std::string& identifier);
};

class FunctionAST
{
public:
    FunctionAST(CType* declaratorType); // append parameter list from FunctionParameter type + extract return type
    ~FunctionAST();
    std::vector<ASTNode*> expressions;
    bool funcDefinedInFile = false;
    bool funcDeclInFile = false;
    std::string identifier;
    ScopeAST scope {.parent = nullptr };
    CType* returnType;
    FunctionPrototype prototype;
};

class CParse
{
public:
    CParse(std::vector<Token>* input);
    ~CParse();
private:
    std::vector<Token>* tokens;
    u32 cursor = 0;
    bool parse();
    bool declarator();
    bool declarationSpecifiers();
};