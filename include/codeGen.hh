#include <cparse.hh>


class CodeGenerator {
public:
    explicit CodeGenerator(AVM &virtualMachineState, std::string fileName);
    void startFinalTranslation();
    ~CodeGenerator();
private:
    AVM& virtualMachine;
    std::ofstream* assemblyFile = nullptr;
    void convertFunctionToASM(AVMFunction* function);
    void convertBasicBlockToASM(AVMBasicBlock* basicBlock);



};