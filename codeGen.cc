#include <codeGen.hh>
CodeGenerator::CodeGenerator(AVM &virtualMachineState, std::string fileName)
        : virtualMachine(virtualMachineState) {
    assemblyFile = new std::ofstream(fileName);
}

CodeGenerator::~CodeGenerator() {
    assemblyFile->close();
    delete assemblyFile;
}

void CodeGenerator::startFinalTranslation() {
    for (auto it : virtualMachine.globalSyms)
    {
        std::string temp;
        if (it->type->isNumVar()||it->type->isPtr()) {
            temp.append(it->identifier);
            temp.append(": ");
            temp.append(".octa ");
            temp.append(std::to_string(it->value));
            temp.append("\n");
        }
#pragma clang diagnostic push
#pragma ide diagnostic ignored "NullDereference"
        *assemblyFile << temp;
#pragma clang diagnostic pop
    }
    for (auto it : virtualMachine.compilationUnit)
    {
        convertFunctionToASM(it);
    }
}
void CodeGenerator::convertFunctionToASM(AVMFunction *function) {
    std::string temp;
    temp.append(function->name);
    temp.append(":\n");
#pragma clang diagnostic push
#pragma ide diagnostic ignored "NullDereference"
    *assemblyFile << temp;
#pragma clang diagnostic pop
    bool finished = false;
    while (!finished)
    {
        for (auto* instruction : function->basicBlocksInFunction.at(0)->sequenceOfInstructions)
        {
            if (instruction->getInstructionType() == AVMInstructionType::ALLOCA)
            {
                finished = true;
                break;
            }
            else {

            }
        }
    }
}

void CodeGenerator::convertBasicBlockToASM(AVMBasicBlock *basicBlock) {

}

