#include <codeGen.hh>
#include <bit>
std::unordered_map<Register, std::string> regToStringMap = {
        {Register::X0, "x0"},
        {Register::X1, "x1"},
        {Register::X2, "x2"},
        {Register::X3, "x3"},
        {Register::X4, "x4"},
        {Register::X5, "x5"},
        {Register::X6, "x6"},
        {Register::X7, "x7"},
        {Register::X8, "x8"},
        {Register::X9, "x9"},
        {Register::X10,"x10"},
        {Register::X11, "x11"},
        {Register::X12, "x12"},
        {Register::X13, "x13"},
        {Register::X14, "x14"},
        {Register::X15, "x15"},
        {Register::X16, "x16"},
        {Register::X17, "x17"},
        {Register::X18, "x18"},
        {Register::X19, "x19"},
        {Register::X20, "x20"},
        {Register::X21, "x21"},
        {Register::X22, "x22"},
        {Register::X23, "x23"},
        {Register::X24, "x24"},
        {Register::X25, "x25"},
        {Register::X26, "x26"},
        {Register::X27, "x27"},
        {Register::X28, "x28"},
        {Register::X29, "x29"},
        {Register::X30,"x30"}
};
std::unordered_map<AVMOpcode, std::string> AVMtoARMv8 {
        {AVMOpcode::ADD, "add"},
        {AVMOpcode::SUB, "sub"},
        {AVMOpcode::MUL, "mul"},
        {AVMOpcode::DIV, "udiv"},
        {AVMOpcode::MOD, "NULL"},
        {AVMOpcode::SLL, "lsl"},
        {AVMOpcode::ASR, "asr"},
        {AVMOpcode::SLR, "lsr"},
        {AVMOpcode::AND, "and"},
        {AVMOpcode::XOR, "eor"},
        {AVMOpcode::ORR, "orr"}
};

std::string regToString(Register registerName)
{
    return regToStringMap[registerName];
}
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
int roundUp(int numToRound)
{
    int remainder = numToRound % 16;
    if (remainder == 0)
        return numToRound;
    return numToRound + 16 - remainder;
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
            allocations.push_back(dynamic_cast<AllocaInstruction*>(instruction));
            functionLocalSymbolMapOnStack.emplace_back(dynamic_cast<AllocaInstruction*>(instruction)->target, varsInitialised);
            varsInitialised++;
        } else {
            finished = true;
            break;
        }
    }
    // treat each as u64,
    // finding out stack size,
    u8 stackSize = 0;
    u8 divisionRes = (varsInitialised*8) / 16;
    u8 remainder = ((varsInitialised*8)%16)/8;
    stackSize = roundUp(varsInitialised*8)/16;
    // Need to do this as stack pointer **MUST** be 16-byte aligned
    assemblyFile << Preparation(stackSize);
    for (auto it : function->basicBlocksInFunction)
    {
        assemblyFile << it->label << ":\n";
        if (it->label == "entry")
        {
                for (auto ins : it->sequenceOfInstructions) // initialise local variables
                {
                    if (ins->getInstructionType() != AVMInstructionType::ALLOCA)
                        break;

                    std::string nameOfVar = dynamic_cast<AllocaInstruction*>(ins)->target;
                    Symbol* symbol = nullptr;
                    for (auto sym : function->variablesInFunction)
                        if (sym->identifier == nameOfVar) {
                            symbol = sym;
                            break;
                        }

                    std::string init{};
                    if (std::__countl_zero(symbol->value) > 48) {
                        init.append("\tmov x9, #");
                        init.append(std::to_string(symbol->value));
                        init.append("\n");
                    }
                    else {
                        init.append("\tldr x9, =");
                        init.append(std::to_string(symbol->value));
                        init.append(" // Initialising local variables \n");
                    }
                    init.append("\tstr x9, [sp, #");
                    u16 idxStack = 0;
                    for (const auto& val : functionLocalSymbolMapOnStack)
                        if (symbol->identifier == val.first) {
                            idxStack = val.second;
                            break;
                        }
                    init.append(std::to_string(8*idxStack));
                    init.append("] // store initial value \n");

                    assemblyFile << init;
            }
        }
        regAllocInit(function);
        convertBasicBlockToASM(it);
    }

}

std::string CodeGenerator::Preparation(u8 stackSize) {
    std::string prelim{};
    u16 stackAllocSize = (stackSize*16) + 16;
    prelim.append("\tsub sp, sp, #");
    prelim.append(std::to_string(stackAllocSize));
    prelim.append(" // Allocate stack space \n\tstp x29, x30, [sp, #");
    prelim.append(std::to_string(stackAllocSize-16));
    prelim.append("] // Save the link register and frame pointer as per convention\n");
    return prelim;
}

void CodeGenerator::convertBasicBlockToASM(AVMBasicBlock *basicBlock) {


    // start of conversion
    /*
     * Rather than keeping local variables on the stack
     *
     *
     * */

    for (auto x = 0; x < basicBlock->sequenceOfInstructions.size(); x++)
    {
        auto it = basicBlock->sequenceOfInstructions.at(x);
        if (it->getInstructionType() == AVMInstructionType::BRANCH)
        {

            break;
        }

        switch (it->getInstructionType())
        {
            case AVMInstructionType::ARITHMETIC: {
                auto findOpcode = AVMtoARMv8.find(it->opcode);
                if (findOpcode->second == "NULL")
                {
                    //complexOpHandler(it);
                }
                auto arithmeticInstruction = dynamic_cast<ArithmeticInstruction*>(it);
                std::string temp{};
                temp.append("\t");
                temp.append(findOpcode->second);
                temp.append(" ");
                temp.append(regToString(allocRegister(arithmeticInstruction->dest)));
                temp.append(", ");
                temp.append(regToString(findVariable(arithmeticInstruction->src1)));
                temp.append(", ");
                temp.append(regToString(findVariable(arithmeticInstruction->src2)));
                temp.append("\n");
                assemblyFile << temp;
            }
            case AVMInstructionType::LOAD:
                break;
            case AVMInstructionType::STORE:
                break;
            case AVMInstructionType::GEP:
                break;
            case AVMInstructionType::CMP:
                break;
            case AVMInstructionType::CALL:
                break;
            case AVMInstructionType::RET:
                break;
            case AVMInstructionType::MV:
                break;
            default:
                break;
        }
    }

}

void CodeGenerator::regAllocInit(AVMFunction* function) {
    freeRegisters.push(Register::X9);
    freeRegisters.push(Register::X10);
    freeRegisters.push(Register::X11);
    freeRegisters.push(Register::X12);
    freeRegisters.push(Register::X13);
    freeRegisters.push(Register::X14);
    freeRegisters.push(Register::X15);
    freeRegisters.push(Register::X16);

    for (auto it : function->variablesInFunction)
    {
        u64 bytecodeLine = 0;
        u64 bytecodeLineFirst = 0;
        u64 bytecodeLineLast = 0;
        for (auto it2 : function->basicBlocksInFunction)
        {
            for (auto it3 : it2->sequenceOfInstructions)
            {
                switch (it3->getInstructionType()) {
                    case AVMInstructionType::ARITHMETIC: {
                        auto instruction = dynamic_cast<ArithmeticInstruction*>(it3);
                        if (instruction->dest == it->identifier || instruction->src1 == it->identifier || instruction->src2 == it->identifier) {
                            if (bytecodeLineFirst==0) {
                                bytecodeLineFirst = bytecodeLine;
                            }
                            else {
                                bytecodeLineLast = bytecodeLine;
                            }
                        }

                        break;
                    }
                    case AVMInstructionType::LOAD: {
                        auto instruction = dynamic_cast<LoadMemoryInstruction*>(it3);
                        if (instruction->dest == it->identifier || instruction->addrVar == it->identifier){
                            if (bytecodeLineFirst==0) {
                                bytecodeLineFirst = bytecodeLine;
                            }
                            else {
                                bytecodeLineLast = bytecodeLine;
                            }
                        }
                        break;
                    }
                    case AVMInstructionType::STORE: {
                        auto instruction = dynamic_cast<StoreMemoryInstruction*>(it3);
                        if (instruction->src == it->identifier || instruction->addrVar == it->identifier){
                            if (bytecodeLineFirst==0) {
                                bytecodeLineFirst = bytecodeLine;
                            }
                            else {
                                bytecodeLineLast = bytecodeLine;
                            }
                        }
                        break;
                    }
                    case AVMInstructionType::GEP: {
                        auto instruction = dynamic_cast<GetElementPtr*>(it3);
                        if (instruction->src == it->identifier || instruction->dest == it->identifier){
                            if (bytecodeLineFirst==0) {
                                bytecodeLineFirst = bytecodeLine;
                            }
                            else {
                                bytecodeLineLast = bytecodeLine;
                            }
                        }
                        break;
                    }
                    case AVMInstructionType::CMP: {
                        auto instruction = dynamic_cast<ComparisonInstruction*>(it3);
                        if (instruction->op1 == it->identifier || instruction->op2 == it->identifier || instruction->dest == it->identifier){
                            if (bytecodeLineFirst==0) {
                                bytecodeLineFirst = bytecodeLine;
                            }
                            else {
                                bytecodeLineLast = bytecodeLine;
                            }
                        }
                        break;
                    }
                    case AVMInstructionType::BRANCH: {
                        auto instruction = dynamic_cast<BranchInstruction*>(it3);
                        if (instruction->dependantComparison == it->identifier){
                            if (bytecodeLineFirst==0) {
                                bytecodeLineFirst = bytecodeLine;
                            }
                            else {
                                bytecodeLineLast = bytecodeLine;
                            }
                        }
                        break;
                    }
                    case AVMInstructionType::CALL: {
                        auto instruction = dynamic_cast<CallInstruction*>(it3);
                        bool present = false;
                        for (const auto& it4 : instruction->args)
                            if (it->identifier == it4)
                                present = true;
                        if (instruction->returnVal == it->identifier || present){
                            if (bytecodeLineFirst==0) {
                                bytecodeLineFirst = bytecodeLine;
                            }
                            else {
                                bytecodeLineLast = bytecodeLine;
                            }
                        }
                        break;
                    }
                    case AVMInstructionType::RET: {
                        auto instruction = dynamic_cast<RetInstruction*>(it3);
                        if (instruction->value == it->identifier){
                            if (bytecodeLineFirst==0) {
                                bytecodeLineFirst = bytecodeLine;
                            }
                            else {
                                bytecodeLineLast = bytecodeLine;
                            }
                        }
                        break;
                    }
                    case AVMInstructionType::MV: {
                        auto instruction = dynamic_cast<MoveInstruction*>(it3);
                        if (instruction->valueToBeMoved == it->identifier || instruction->dest == it->identifier){
                            if (bytecodeLineFirst==0) {
                                bytecodeLineFirst = bytecodeLine;
                            }
                            else {
                                bytecodeLineLast = bytecodeLine;
                            }
                        }
                        break;
                    }
                    case AVMInstructionType::ALLOCA:
                    {
                        auto instruction = dynamic_cast<AllocaInstruction*>(it3);
                        if (instruction->target == it->identifier){
                            if (bytecodeLineFirst==0) {
                                bytecodeLineFirst = bytecodeLine;
                            }
                            else {
                                bytecodeLineLast = bytecodeLine;
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
                bytecodeLine++;

            }

        }
        liveRanges[it->identifier] = {bytecodeLineFirst, bytecodeLineLast};
    }
}

Register CodeGenerator::findVariable(std::string) {
    return Register::X19;
}

Register CodeGenerator::allocRegister(std::string) {
    return Register::X19;
}

