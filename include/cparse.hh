#pragma once
#include <sfce.hh>
#include <lexer.hh>
#include <memory>
#include <graph.hh>
struct ScopeAST;
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
    A_END,
    A_SLR
};
ASTop isBinOp(Token& token);
bool ASTopIsBinOp(ASTop op);
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
    virtual ~DeclaratorPieces() = default;
    virtual std::string print();
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
    std::string print() override {
        std::string temp;
        temp.append("*");
        if (isConst)
            temp.append("const");
        if (isVolatile)
            temp.append("volatile");
        return temp;
    }
private:
    DeclaratorPieceType dpt = PTR;
    bool isConst = false;
    bool isVolatile = false;
};

struct CType
{
    ~CType() {
        for (auto* i : declaratorPartList)
            delete i;
    };
    std::vector<Token> typeSpecifier;
    std::vector<DeclaratorPieces*> declaratorPartList;
    char bitfield = 0; // Used for unions, currently unsupported **TODO**

    void copy(CType* x);
    bool isCompatible(CType type, ASTop op);
    bool isPtr();
    bool isArray();
    bool isNumVar();
    bool isFuncPtr();
    bool isEqual(CType* otherType, bool ptrOrNum);
    CType* dereferenceType();
    CType* refType();
    std::string typeAsString();

private:
    bool assignCompat(CType type);
};
struct Symbol;
class FunctionPrototype : public DeclaratorPieces
{
public:
    ~FunctionPrototype() override;
    DeclaratorPieceType getDPT() final {return dpt;};
    ScopeAST* scope = nullptr;
    bool doNotDeleteScope = false;
    std::vector<Symbol*> types{};
    std::string print() override;
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
    ~Symbol() {
        delete type;
    }
    u64 value = 0;
    CType* type = nullptr;
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
    CType* type = nullptr; // only usable if op = A_TYPE_CVT
    ScopeAST* scope = nullptr;
    bool doNotDeleteType = false;
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
    std::unordered_map<std::string, u32> SymbolHashMap; // Map function/prototypes to ids in an array
};

struct ScopeAST {
    ScopeAST* parent = nullptr;
    ~ScopeAST() = default;
    RegularSymbolTable rst;

    i64 findRegularSymbol(const std::string& identifier);
    i64 findSymbolInLocalScope(const std::string& identifier);
    i64 findEarliestScopeLevel(i64 startVal, const std::string &identifier);
};
bool isTypeSpecifier(const Token& token);
bool isTypeQualifier(const Token& token);
int sizeOf(std::vector<Token>& typeSpecifiers);
class FunctionAST
{
public:
    explicit FunctionAST(const std::string& identifier)
    {
        funcIdentifier = identifier;
    }; // append parameter list from FunctionParameter type + extract return type
    ~FunctionAST() = default;
    ASTNode* root = nullptr;
    void printFunction() const {
        ASTNode::print(root);
    }
    bool funcDefinedInFile = false;
    bool funcDeclInFile = false;
    std::string funcIdentifier;
    u32 globalSymTableIdx = 0;

};

class CParse
{
public:
    friend class SemanticAnalyser;
    friend class AVM;
    explicit CParse(std::vector<Token>* input);
    bool parse();
    std::vector<FunctionAST*> functions;
    ~CParse();

private:
    std::vector<Token>* tokens;
    u32 cursor = 0;
    ScopeAST* currentScope = nullptr;
    FunctionAST* currentFunction{};
    std::vector<Symbol*> globalSymbolTable{};
    u64 globalIndex = 0;



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




    std::string identifier;
    bool fuse = false;
};

class SemanticAnalyser {
public:
    SemanticAnalyser();
    ~SemanticAnalyser();
    ScopeAST* scope = nullptr;
    FunctionAST* currentFunction = nullptr;

    bool analyseFunction(CParse& parserState, FunctionAST* function);

    bool analyseTree(CParse& parserState, ASTNode* root);

    CType * evalType(CParse& parserState, ASTNode* expr);

    bool startSemanticAnalysis(CParse &parserState);

    CType *normaliseTypes(CType *LHS, CType *RHS) const;
};
enum class AVMOpcode {
    // Arithmetic class
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    SLL,
    SLR,
    ASR,
    AND,
    ORR,
    XOR,
    // Memory class
    GEP,
    LD,
    ST,
    // Control Flow class
    CMP,
    CSEL,
    BR,
    RET,
    CALL,
    // Miscellaneous
    MV,
    NOP,
    PROGEND
};
enum class CMPCode {
    LT,
    MT,
    LTEQ,
    MTEQ,
    EQ,
    NEQ,
    NC
};

enum class AVMInstructionType {
    ARITHMETIC,
    LOAD,
    STORE,
    GEP,
    CMP,
    BRANCH,
    CALL,
    RET,
    MV,
    END
};
std::string mapOptoString(AVMOpcode op);
std::string mapConditionCodetoString(CMPCode code);
AVMOpcode toAVM(ASTop op);
bool ASTopIsBinOpAVM(ASTop op);
class AVMInstruction {
public:
    virtual AVMInstructionType getInstructionType() {
        return AVMInstructionType::ARITHMETIC;
    }

    virtual std::string print() {
        return {};
    }
    AVMOpcode opcode = AVMOpcode::NOP;
};


class LoadMemoryInstruction : public AVMInstruction {
public:
    AVMInstructionType getInstructionType() override {
        return type;
    }
    std::string dest{};
    std::string addrVar{};
    std::string print() override {
        std::string temp{};
        temp.append("ldr ");
        temp.append(dest);
        temp.append(",");
        temp.append(addrVar);
        return temp;
    }
private:
    AVMInstructionType type = AVMInstructionType::LOAD;
};
class StoreMemoryInstruction : public AVMInstruction {
public:
    AVMInstructionType getInstructionType() override {
        return type;
    }
    std::string src{};
    std::string addrVar{};
    std::string print() override {
        std::string temp{};
        temp.append("str ");
        temp.append(src);
        temp.append(",");
        temp.append(addrVar);
        return temp;
    }
private:
    AVMInstructionType type = AVMInstructionType::STORE;
};
class GetElementPtr : public AVMInstruction {
public:
    AVMInstructionType getInstructionType() override {
        return type;
    }
    std::string dest{};
    std::string src{};
    std::string print() override {
        std::string temp{};
        temp.append("gep ");
        temp.append(dest);
        temp.append(",");
        temp.append(src);
        return temp;
    }
private:
    AVMInstructionType type = AVMInstructionType::GEP;
};
class ComparisonInstruction : public AVMInstruction {
public:
    AVMInstructionType getInstructionType() override {
        return type;
    }
    std::string dest{};
    std::string op1{};
    std::string op2{};
    CMPCode compareCode = CMPCode::NC;
    std::string print() override {
        std::string temp{};
        temp.append("cmp.");
        temp.append(mapConditionCodetoString(compareCode));
        temp.append(" ");
        temp.append(dest);
        temp.append(",");
        temp.append(op1);
        temp.append(",");
        temp.append(op2);
        return temp;
    }
private:
    AVMInstructionType type = AVMInstructionType::CMP;
};
class RetInstruction : public AVMInstruction {
public:
    AVMInstructionType getInstructionType() override {
        return type;
    }
    std::string value{};
    std::string print() override {
        std::string temp{};
        temp.append("ret ");
        temp.append(value);
        return temp;
    }
private:
    AVMInstructionType type = AVMInstructionType::RET;
};
class ArithmeticInstruction : public AVMInstruction {
public:
    AVMInstructionType getInstructionType() override {
        return type;
    }
    std::string dest{};
    std::string src1{};
    std::string src2{};
    std::string print() override {
        std::string temp{};
        temp.append(mapOptoString(opcode));
        temp.append(" ");
        temp.append(dest);
        temp.append(",");
        temp.append(src1);
        temp.append(",");
        temp.append(src2);
        return temp;
    }
private:
    AVMInstructionType type = AVMInstructionType::ARITHMETIC;
};
bool ASTopIsCMPOp(ASTop op);
CMPCode toCMPCode(ASTop op);
class CallInstruction : public AVMInstruction {
public:
    AVMInstructionType getInstructionType() override {
        return type;
    }
    std::string returnVal{};
    std::string funcName{};
    std::vector<std::string> args;
    std::string print() override {
        std::string temp{};
        temp.append(returnVal);
        temp.append(" = ");
        temp.append("call ");
        temp.append(funcName);
        temp.append("(");
        for (auto& i : args) {
            temp.append(i);
            temp.append(",");
        }
        temp.erase(temp.end()-1);
        temp.append(")");
        return temp;
    }
private:
    AVMInstructionType type = AVMInstructionType::CALL;
};

class MoveInstruction : public AVMInstruction {
public:
    AVMInstructionType getInstructionType() override {
        return type;
    }
    std::string dest{};
    std::string valueToBeMoved{};
    std::string print() override
    {
        std::string temp{};
        temp.append("mov ");
        temp.append(dest);
        temp.append(",");
        temp.append(valueToBeMoved);
        return temp;
    }
private:
    AVMInstructionType type = AVMInstructionType::MV;
};
class BranchInstruction : public AVMInstruction {
public:
    AVMInstructionType getInstructionType() override {
        return type;
    }
    std::string falseTarget{};
    std::string trueTarget{};
    std::string dependantComparison{}; // #1 means unconditional branch on the trueTarget
    std::string print() override
    {
        std::string temp{};
        temp.append("br ");
        temp.append(dependantComparison);
        temp.append(" true: ");
        temp.append(trueTarget);
        temp.append(" false: ");
        temp.append(falseTarget);
        return temp;
    }
private:
    AVMInstructionType type = AVMInstructionType::BRANCH;
};
class ProgramEndInstruction : public AVMInstruction
{
public:
    AVMInstructionType getInstructionType() override {
        return type;
    }
    std::string print() override
    {
        std::string temp{};
        temp.append("end");
        return temp;
    }
private:
    AVMInstructionType type = AVMInstructionType::END;
};
class CSELInstruction : public AVMInstruction {};

class AVMBasicBlock {
public:
    ~AVMBasicBlock();
    std::vector<AVMInstruction*> sequenceOfInstructions;
    std::string label;
    std::string print() {
        std::string temp{};
        temp.append("\t");
        for (auto* i : sequenceOfInstructions)
        {
            temp.append(i->print());
            temp.append("\n");
            temp.append("\t");
        }
        temp.erase(temp.end()-1);
        return temp;
    }
};

class AVMFunction {
public:
    ~AVMFunction();
    std::vector<Symbol*> incomingSymbols;
    std::vector<AVMBasicBlock*> basicBlocksInFunction;
    AdjacencyMatrix* adjacencyMatrix = nullptr;
private:

};
struct AVMBasicBlocksForIFs {
    AVMBasicBlock* truePath;
    AVMBasicBlock* falsePath;
};
class AVM {
public:
    explicit AVM(CParse &parserState);
    ~AVM();
    AVMBasicBlock* currentBasicBlock = nullptr;
    CParse& parserState;
    std::vector<AVMFunction*> compilationUnit{};
    void AVMByteCodeDriver(FunctionAST* functionToBeTranslated);
    void cvtBasicBlockToAVMByteCode(ASTNode *node, AVMBasicBlock* basicBlock);
    std::vector<Symbol*> globalSyms;
    AVMFunction* currentFunction = nullptr;
    ASTNode* currentNode = nullptr;
    std::string genCode(ASTNode* expr);
    std::string genTmpDest() {
        std::string tmp = "%tmp.";
        tmp.append(std::to_string(tmpCounter));
        tmpCounter++;
        return tmp;
    }
    std::vector<std::string> genArgs(ASTNode* argNode)
    {
        if (argNode == nullptr)
            return {};
        std::vector<std::string> temp;
        if (argNode->op == A_GLUE)
            temp.push_back(argNode->left->identifier);
        else
            temp.push_back(argNode->identifier);
        auto genArgs2 = genArgs(argNode->right);
        temp.insert(temp.end(), genArgs2.begin(), genArgs2.end());
        return temp;
    }
    u64 labelCounter = 0;
    std::string genLabel() {
        std::string tmp = "@L.";
        tmp.append(std::to_string(labelCounter));
        labelCounter++;
        return tmp;
    }
    AVMBasicBlocksForIFs* ifHandler(ASTNode* expr);
    u64 tmpCounter = 0;
};