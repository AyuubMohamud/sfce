#include <cparse.hh>
#include <queue>
enum class Register {
    X0,
    X1,
    X2,
    X3,
    X4,
    X5,
    X6,
    X7,
    X8,
    X9,
    X10,
    X11,
    X12,
    X13,
    X14,
    X15,
    X16,
    X17,
    X18,
    X19,
    X20,
    X21,
    X22,
    X23,
    X24,
    X25,
    X26,
    X27,
    X28,
    X29,
    X30
};


class CodeGenerator {
public:
    explicit CodeGenerator(AVM &virtualMachineState, std::string fileName);
    void startFinalTranslation();
    ~CodeGenerator();
private:
    AVM& virtualMachine;
    std::ofstream assemblyFile;
    void convertFunctionToASM(AVMFunction* function);
    void convertBasicBlockToASM(AVMBasicBlock* basicBlock);
    std::vector<std::pair<std::string, u16>> functionLocalSymbolMapOnStack;
    std::vector<std::pair<std::string, Register>> functionRegisterMap;
    std::vector<std::pair<std::string, bool>> functionLocalVarIsOnStack;

    std::unordered_map<std::string, std::pair<u64,u64>> liveRanges;
    std::queue<Register> freeRegisters;

    std::string Prologue(u32 stackSize);

    Register findVariable(std::string);
    Register allocRegister(std::string);
    void regAllocInit(AVMFunction* function);

    void freeRegs();

    void saveVariable(const std::string& identifier);

    std::string Epilogue(u32 stackSize);
};