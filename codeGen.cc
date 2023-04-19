#include <codeGen.hh>
CodeGenerator::CodeGenerator(AVM &virtualMachineState, std::string fileName)
        : virtualMachine(virtualMachineState) {
    assemblyFile.open(fileName);
}

CodeGenerator::~CodeGenerator() {
    assemblyFile.close();
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
        assemblyFile << temp;
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
    assemblyFile << temp;
#pragma clang diagnostic pop
    bool finished = false;
    std::vector<AllocaInstruction*> allocations;
    int varsInitialised = 0;
    for (auto* instruction : function->basicBlocksInFunction.at(0)->sequenceOfInstructions) {
        if (instruction->getInstructionType() == AVMInstructionType::ALLOCA) {
            varsInitialised++;
            allocations.push_back(dynamic_cast<AllocaInstruction*>(instruction));
        } else {
            finished = true;
            break;
        }
    }
    for (auto* symbol : function->incomingSymbols) // incoming symbols are also saved on the stack
    {
        varsInitialised++;
    }
    // treat each as u64,
    u8 stackSize = 0;
    u8 divisionRes = (varsInitialised*8) / 16;
    u8 remainder = (varsInitialised*8)%16;
    stackSize = divisionRes + remainder;
    // Need to do this as stack pointer **MUST** be 16-byte aligned
    assemblyFile << Preparation(stackSize);


}

std::string CodeGenerator::Preparation(u8 stackSize) {
    std::string prelim{};
    u16 stackAllocSize = (stackSize*8) + 16;
    prelim.append("\tsub sp, sp, #");
    prelim.append(std::to_string(stackAllocSize));
    prelim.append(" // Allocate stack space \n\tstp x29, x30, [sp, #");
    prelim.append(std::to_string(stackAllocSize-16));
    prelim.append("] // Save the link register and frame pointer as per convention\n");
    return prelim;
}

void CodeGenerator::convertBasicBlockToASM(AVMBasicBlock *basicBlock) {

}

