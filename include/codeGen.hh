#include <cparse.hh>


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
    std::vector<std::pair<std::string, u16>> functionLocalSymbols;

    std::string Preparation(u8 stackSize);
};