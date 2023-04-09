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

    bool isCompatible(CType type, ASTop op);
    bool isPtr();
    bool isArray();
    bool isNumVar();
    bool isFuncPtr();
    bool isEqual(CType* otherType, bool ptrOrNum);
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
    ~CParse();

private:
    std::vector<Token>* tokens;
    u32 cursor = 0;
    ScopeAST* currentScope = nullptr;
    std::vector<FunctionAST*> functions;
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

    CType *normaliseTypes(CType *LHS, CType *RHS);
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
    NOP
};
enum class CMPCode {
    LT,
    MT,
    LTEQ,
    MTEQ,
    EQ,
    NEQ
};
std::unordered_map<CMPCode, std::string> CMPtoString {
        {CMPCode::LT, "LT"},
        {CMPCode::MT, "MT"},
        {CMPCode::LTEQ, "LTEQ"},
        {CMPCode::MTEQ, "MTEQ"},
        {CMPCode::EQ, "EQ"},
        {CMPCode::NEQ, "NEQ"}
};
std::unordered_map<AVMOpcode, std::string> OpcodeToString {
        {AVMOpcode::ADD, "ADD"},
        {AVMOpcode::SUB, "SUB"},
        {AVMOpcode::MUL, "MUL"},
        {AVMOpcode::DIV, "DIV"},
        {AVMOpcode::MOD, "MOD"},
        {AVMOpcode::SLL, "SLL"},
        {AVMOpcode::ASR, "ASR"},
        {AVMOpcode::SLR, "SLR"},
        {AVMOpcode::AND, "AND"},
        {AVMOpcode::XOR, "XOR"},
        {AVMOpcode::ORR, "ORR"}
};
std::unordered_map<ASTop, AVMOpcode> ASTOperationToAVM {
        {A_ADD, AVMOpcode::ADD},
        {A_SUB, AVMOpcode::SUB},
        {A_MULT, AVMOpcode::MUL},
        {A_DIV, AVMOpcode::DIV},
        {A_MODULO, AVMOpcode::MOD},
        {A_SLL, AVMOpcode::SLL},
        {A_ASR, AVMOpcode::ASR},
        {A_SLR, AVMOpcode::SLR},
        {A_AND, AVMOpcode::AND},
        {A_XOR, AVMOpcode::XOR},
        {A_OR, AVMOpcode::ORR}
};

std::string mapOptoString(AVMOpcode op) {
    auto value = OpcodeToString.find(op);
    if (value == OpcodeToString.end())
        return {};
    return value->second;
}
std::string mapConditionCodetoString(CMPCode code) {
    auto value = CMPtoString.find(code);
    if (value == CMPtoString.end())
        return {};
    return value->second;
}

AVMOpcode toAVM(ASTop op)
{
    auto value = ASTOperationToAVM.find(op);
    if (value == ASTOperationToAVM.end())
        return AVMOpcode::NOP;
    return value->second;
}
bool ASTopIsBinOpAVM(ASTop op) {
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
            || op == A_SLR
            || op == A_OR
            || op == A_XOR
    );
}
enum class AVMInstructionType {
    ARITHMETIC,
    LOAD,
    STORE,
    GEP,
    CMP,
    BRANCH,
    CALL,
    RET,
    MV
};
class AVMInstruction {
public:
    virtual AVMInstructionType getInstructionType();
    virtual std::string print();
    AVMOpcode opcode = AVMOpcode::NOP;
};

std::string AVMInstruction::print() {
    return std::string();
}

AVMInstructionType AVMInstruction::getInstructionType() {
    return AVMInstructionType::ARITHMETIC;
}

class LoadMemoryInstruction : public AVMInstruction {
public:
    AVMInstructionType getInstructionType() override {
        return type;
    }
    std::string dest{};
    std::string addrVar{};
    std::string print() override {
        std::string temp{};
        temp.append(dest);
        temp.append(" = LOAD");
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
        temp.append("STORE ");
        temp.append(src);
        temp.append(" , ");
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
        temp.append(dest);
        temp.append(" = GEP ");
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
    CMPCode compareCode = CMPCode::EQ;
    std::string print() override {
        std::string temp{};
        temp.append(dest);
        temp.append("= CMP ");
        temp.append(mapConditionCodetoString(compareCode));
        temp.append(" ");
        temp.append(op1);
        temp.append(" , ");
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
        temp.append("RET ");
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
        temp.append(dest);
        temp.append("= ");
        temp.append(mapOptoString(opcode));
        temp.append(" ");
        temp.append(src1);
        temp.append(" , ");
        temp.append(src2);
        return temp;
    }
private:
    AVMInstructionType type = AVMInstructionType::ARITHMETIC;
};
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
        temp.append("= ");
        temp.append("CALL ");
        temp.append(funcName);
        temp.append(" (");
        for (auto& i : args) {
            temp.append(i);
            temp.append(",");
        }
        temp.erase(temp.end());
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
        temp.append(dest);
        temp.append(" = MV ");
        temp.append(valueToBeMoved);
        return temp;
    }
private:
    AVMInstructionType type = AVMInstructionType::MV;
};
class BranchInstruction : public AVMInstruction {
    AVMInstructionType getInstructionType() override {
        return type;
    }
    std::string falseTarget{};
    std::string trueTarget{};
    std::string dependantComparison{}; // #1 means unconditional branch on the trueTarget
    std::string print() override
    {
        std::string temp{};
        temp.append("BR ");
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
class CSELInstruction : public AVMInstruction {};

class AVMBasicBlock {
public:
    std::vector<AVMInstruction*> sequenceOfInstructions;
    std::string print() {
        std::string temp{};
        for (auto* i : sequenceOfInstructions)
        {
            temp.append(i->print());
            temp.append("\n");
        }
        return temp;
    }
};

class AVMFunction {
public:
    std::vector<Symbol*> incomingSymbols;
    std::vector<AVMBasicBlock*> basicBlocksInFunction;
    AdjacencyMatrix* adjacencyMatrix = nullptr;
private:

};

class AVM {
public:
    explicit AVM(CParse &parserState);
    ~AVM() = default;
    AVMBasicBlock* currentBasicBlock = nullptr;
    CParse& parserState;
    std::vector<AVMFunction*> compilationUnit{};
    void AVMByteCodeDriver(FunctionAST* functionToBeTranslated);
    AVMBasicBlock *cvtBasicBlockToAVMByteCode(ASTNode *node);

    AVMFunction* currentFunction = nullptr;
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
        temp.push_back(argNode->left->identifier);
        auto genArgs2 = genArgs(argNode->right);
        temp.insert(temp.end(), genArgs2.begin(), genArgs2.end());
        return temp;
    }
    u64 tmpCounter = 0;
};