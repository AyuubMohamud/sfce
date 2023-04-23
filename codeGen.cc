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
    assemblyFile << temp;
    bool finished = false;
    std::vector<AllocaInstruction*> allocations;
    int varsInitialised = 0;
    for (auto* instruction : function->basicBlocksInFunction.at(0)->sequenceOfInstructions) {
        if (instruction->getInstructionType() == AVMInstructionType::ALLOCA) {

        }
        switch (instruction->getInstructionType()) {
            case AVMInstructionType::ARITHMETIC: {
                functionLocalSymbolMapOnStack.emplace_back(dynamic_cast<ArithmeticInstruction*>(instruction)->dest, varsInitialised);
                varsInitialised++;
                break;
            }
            case AVMInstructionType::LOAD: {
                functionLocalSymbolMapOnStack.emplace_back(dynamic_cast<LoadMemoryInstruction*>(instruction)->dest, varsInitialised);
                varsInitialised++;
                break;
            }
            case AVMInstructionType::STORE:
                break;
            case AVMInstructionType::GEP:
            {
                functionLocalSymbolMapOnStack.emplace_back(dynamic_cast<GetElementPtr*>(instruction)->dest, varsInitialised);
                varsInitialised++;
                break;
            }
            case AVMInstructionType::CMP: {
                functionLocalSymbolMapOnStack.emplace_back(dynamic_cast<ComparisonInstruction*>(instruction)->dest, varsInitialised);
                varsInitialised++;
                break;
            }
            case AVMInstructionType::BRANCH: {

                break;
            }
            case AVMInstructionType::CALL: {
                functionLocalSymbolMapOnStack.emplace_back(dynamic_cast<CallInstruction*>(instruction)->returnVal, varsInitialised);
                varsInitialised++;
                break;
            }
            case AVMInstructionType::RET:
                break;
            case AVMInstructionType::MV: {
                functionLocalSymbolMapOnStack.emplace_back(dynamic_cast<MoveInstruction*>(instruction)->dest, varsInitialised);
                varsInitialised++;
                break;
            }
            case AVMInstructionType::ALLOCA:
            {
                allocations.push_back(dynamic_cast<AllocaInstruction*>(instruction));
                functionLocalSymbolMapOnStack.emplace_back(dynamic_cast<AllocaInstruction*>(instruction)->target, varsInitialised);
                varsInitialised++;
                break;
            }
            case AVMInstructionType::END:
                break;
        }
    }
    // treat each as u64,
    // finding out stack size,
    u8 stackSize = 0;
    u8 divisionRes = (varsInitialised*8) / 16;
    u8 remainder = ((varsInitialised*8)%16)/8;
    stackSize = roundUp(varsInitialised*8);
    // Need to do this as stack pointer **MUST** be 16-byte aligned
    assemblyFile << Prologue(stackSize);
    regAllocInit(function);
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
        convertBasicBlockToASM(it);
    }
    assemblyFile << Epilogue(stackSize);
}

std::string CodeGenerator::Prologue(u32 stackSize) {
    std::string prelim{};
    u16 stackAllocSize = (stackSize) + 16;
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
                saveVariable(arithmeticInstruction->dest);
                freeRegs();
                break;
            }
            case AVMInstructionType::LOAD: {
                auto loadInstruction = dynamic_cast<LoadMemoryInstruction*>(it);
                std::string temp{};
                temp.append("ldr ");
                temp.append(regToString(allocRegister(loadInstruction->dest)));
                temp.append(", ");
                temp.append(regToString(findVariable(loadInstruction->addrVar)));
                temp.append("\n");
                assemblyFile << temp;
                saveVariable(loadInstruction->dest);
                freeRegs();
                break;
            }
            case AVMInstructionType::STORE: {
                break;
            }
            case AVMInstructionType::GEP: {
                auto gepInstruction = dynamic_cast<GetElementPtr*>(it);
                std::string temp{};
                for (const auto& symbol : functionLocalSymbolMapOnStack)
                {
                    if (symbol.first == gepInstruction->src)
                    {
                        u16 offset = symbol.second;
                        temp.append("\tadd ");
                        temp.append(regToString(allocRegister(gepInstruction->dest)));
                        temp.append(", sp, #");
                        temp.append(std::to_string(offset));
                        temp.append("\n");
                        assemblyFile << temp;
                        saveVariable(gepInstruction->dest);
                        freeRegs();
                    }
                }
                break;
            }
            case AVMInstructionType::CMP:
                break;
            case AVMInstructionType::CALL:
            {
                auto callInstruction = dynamic_cast<CallInstruction*>(it);
                std::vector<Register> vecOfAvailableRegisters{
                    Register::X0,
                    Register::X1,
                    Register::X2,
                    Register::X3,
                    Register::X4,
                    Register::X5,
                    Register::X6,
                    Register::X7
                };
                u8 cursor = 0;
                for (const auto& symbol : callInstruction->args)
                {
                    std::string temp;
                    temp.append("\tmov ");
                    temp.append(regToString(vecOfAvailableRegisters.at(cursor)));
                    temp.append(", ");
                    temp.append(regToString(findVariable(symbol)));
                    temp.append("\n");
                    assemblyFile << temp;
                    freeRegs();
                    cursor++;
                }
                std::string functionName;
                functionName = callInstruction->funcName;
                functionName.erase(functionName.begin());
                assemblyFile << ("\tbl ") << functionName << "\n";
                assemblyFile << "\tmov x10, x0\n";
                saveVariable(callInstruction->returnVal);
                freeRegs();
                break;
            }
            case AVMInstructionType::RET: {
                auto returnInstruction = dynamic_cast<RetInstruction*>(it);
                std::string moveInstruction{};
                moveInstruction.append("\tmov x0, ");
                moveInstruction.append(regToString(findVariable(returnInstruction->value)));
                assemblyFile << moveInstruction << "\n";
                assemblyFile << "\tret\n";
                break;
            }
            case AVMInstructionType::MV: {
                auto moveInstruction = dynamic_cast<MoveInstruction*>(it);
                std::string temp{};
                temp.append("\tmov ");
                temp.append(regToString(allocRegister(moveInstruction->dest)));
                temp.append(", ");
                temp.append(regToString(findVariable(moveInstruction->valueToBeMoved)));
                temp.append("\n");
                assemblyFile << temp;
                saveVariable(moveInstruction->dest);
                freeRegs();
                break;
            }
            default:
                break;
        }
    }

}


Register CodeGenerator::findVariable(std::string identifier) {
    Register freeReg = freeRegisters.front();
    freeRegisters.pop();
    std::string loadInstruction;
    loadInstruction.append("\tldr ");
    loadInstruction.append(regToString(freeReg));

    u16 offset = 0;
    if (identifier.at(0) != '#') {
        loadInstruction.append(", [sp, ");
        for (const auto &symbol: functionLocalSymbolMapOnStack) {
            if (symbol.first == identifier)
                offset = symbol.second;
        }
        loadInstruction.append("#");
        loadInstruction.append(std::to_string(offset*8));
        loadInstruction.append("]");
    }
    else {
        loadInstruction.append(", =");
        std::string temp = identifier;
        temp.erase(0, 1);
        loadInstruction.append(temp);
    }

    loadInstruction.append("\n");
    assemblyFile << loadInstruction;
    return freeReg;
}
void CodeGenerator::freeRegs() {
    if (freeRegisters.empty()) {
        freeRegisters.push(Register::X9);
        freeRegisters.push(Register::X10);
    }
}
void CodeGenerator::saveVariable(const std::string& identifier) {
    for (const auto& symbol : functionLocalSymbolMapOnStack)
    {
        if (symbol.first == identifier)
        {
            std::string storeInstruction;
            storeInstruction.append("\tstr x10, [sp, #");
            storeInstruction.append(std::to_string(symbol.second*8));
            storeInstruction.append("]\n");
            assemblyFile << storeInstruction;
        }
    }

}
Register CodeGenerator::allocRegister(std::string identifier) {
    return Register::X10;
}

void CodeGenerator::regAllocInit(AVMFunction *function) {
    freeRegs();
}
std::string CodeGenerator::Epilogue(u32 stackSize)
{
    std::string tmp;
    tmp.append("\tldp x29, x30, [sp, #");
    tmp.append(std::to_string(stackSize));
    tmp.append("]\n");
    tmp.append("\tadd sp, sp, #");
    tmp.append(std::to_string(stackSize+16));
    tmp.append("\n");
    return tmp;
}
