#include <sfce.hh>
#include <cparse.hh>
#include <errorHandler.hh>
#include <numeric>
#include <cmath>

/*
 *
 * Traverse AST, ignore A_GLUES
 * genCode whenever we see BinOP, INC, DEC, WHILEDECL, AGEN
 *
 * */







void AVM::AVMByteCodeDriver(FunctionAST* functionToBeTranslated) {
    auto* function = new AVMFunction;
    auto* funcSymbol = parserState.globalSymbolTable[functionToBeTranslated->globalSymTableIdx];
    auto* prototype = dynamic_cast<FunctionPrototype*>(funcSymbol->type->declaratorPartList[1]);
    function->prototype = prototype;
    currentFunction = function;
    for (auto* i : prototype->types) {
        currentFunction->incomingSymbols.push_back(i);
    }
    label = "entry";
    tmpCounter = 0;
    function->name = functionToBeTranslated->funcIdentifier;
    startBasicBlockConversion(functionToBeTranslated->root);
    compilationUnit.push_back(function);
}
/*
void AVM::cvtBasicBlockToAVMByteCode(ASTNode* node, AVMBasicBlock* basicBlock) {
    AVMBasicBlock* previous = nullptr;
    previous = currentBasicBlock;
    currentBasicBlock = basicBlock;
    auto res = genCode(node);
    currentFunction->basicBlocksInFunction.push_back(basicBlock);
    ASTNode* cNode = nullptr;
    if (currentNode == nullptr)
    {
        return;
    }
    switch (currentNode->left->op) {
        case A_IFDECL:
        {
            cNode = currentNode;
            currentNode = currentNode->right;
            auto* ifblocks = ifHandler(cNode->left);
            delete ifblocks;
            break;
        }
        default:
            break;
    }
    currentBasicBlock = previous;
}
*/
void AVM::startBasicBlockConversion(ASTNode* node) {
    auto* entryBasicBlock = new AVMBasicBlock;
    entryBasicBlock->label = "entry";
    currentFunction->basicBlocksInFunction.push_back(entryBasicBlock);
    currentBasicBlock = entryBasicBlock;

    genCode(node);
}

std::string AVM::genCode(ASTNode *expr) {
    switch (expr->op) {
        case A_INC:
        {
            auto* incInstruction = new ArithmeticInstruction;
            std::string dest = genCode(expr->left);

            incInstruction->dest = dest;
            incInstruction->src1 = dest;
            if (expr->value == 0)
                incInstruction->src2 = "#1";
            else {
                incInstruction->src2 = "#";
                incInstruction->src2.append(std::to_string(expr->value));
            }
            incInstruction->opcode = AVMOpcode::ADD;
            currentBasicBlock->sequenceOfInstructions.push_back(incInstruction);
            return dest;
        }
        case A_DEC:
        {
            auto* incInstruction = new ArithmeticInstruction;
            std::string dest = genCode(expr->left);
            incInstruction->dest = dest;
            incInstruction->src1 = dest;
            if (expr->value == 0)
                incInstruction->src2 = "#1";
            else {
                incInstruction->src2 = "#";
                incInstruction->src2.append(std::to_string(expr->value));
            }
            incInstruction->opcode = AVMOpcode::SUB;
            currentBasicBlock->sequenceOfInstructions.push_back(incInstruction);
            return dest;
        }
        case A_DEREF:
        {
            auto* dereference = new LoadMemoryInstruction;
            dereference->opcode = AVMOpcode::LD;
            dereference->addrVar = genCode(expr->left);
            dereference->dest = genTmpDest();
            currentBasicBlock->sequenceOfInstructions.push_back(dereference);
            return dereference->dest;
        }
        case A_AGEN:
        {
            auto* addressGeneration = new GetElementPtr;
            addressGeneration->opcode = AVMOpcode::GEP;
            addressGeneration->src = genCode(expr->left);
            addressGeneration->dest = genTmpDest();
            currentBasicBlock->sequenceOfInstructions.push_back(addressGeneration);
            return addressGeneration->dest;
        }
        case A_IDENT:
        {
            bool found = false;
            for (auto it : globalSyms)
            {
                if (expr->identifier == it->identifier) {
                    found = true;
                    break;
                }
            }
            if (found)
            {
                std::string temp;
                temp.append("@");
                temp.append(expr->identifier);
                return temp;
            }
            return expr->identifier;

        }
        case A_INTLIT:
        {
            std::string tmp{};
            tmp.append("#");
            tmp.append(expr->identifier);
            return tmp;
        }
        case A_RET:
        {
            auto* retInstruction = new RetInstruction;
            if (expr->left != nullptr)
            {
                retInstruction->value = genCode(expr->left);
            }

            retInstruction->opcode = AVMOpcode::RET;
            currentBasicBlock->sequenceOfInstructions.push_back(retInstruction);
            return {};
        }
        case A_CALL:
        {
            auto* callInstruction = new CallInstruction;
            callInstruction->funcName = genCode(expr->left);
            callInstruction->returnVal = genTmpDest();
            callInstruction->opcode = AVMOpcode::CALL;
            // treat args specially don't just gencode
            callInstruction->args = genArgs(expr->right);
            currentBasicBlock->sequenceOfInstructions.push_back(callInstruction);
            return callInstruction->returnVal;
        }
        case A_GLUE:
        {
            auto string = genCode(expr->left);
            if (string == "NBB")
            {
                newBasicBlockHandler(expr->left, expr->right, false);
                return {};
            }
            genCode(expr->right);
            // Since its glue they are not connected, simply ignore their values
            return {};
        }
        case A_CS:
        {
            for (const auto& it : expr->scope->rst.SymbolHashMap)
            {
                currentFunction->variablesInFunction.push_back(parserState.globalSymbolTable.at(it.second));
                auto* allocaInstruction = new AllocaInstruction;
                allocaInstruction->target = it.first;
                currentBasicBlock->sequenceOfInstructions.push_back(allocaInstruction);
            }
            return genCode(expr->left);
        }
        case A_MV:
        {
            auto* moveInstruction = new MoveInstruction;
            moveInstruction->valueToBeMoved = genCode(expr->right);
            moveInstruction->opcode = AVMOpcode::MV;
            moveInstruction->dest = genCode(expr->left);
            currentBasicBlock->sequenceOfInstructions.push_back(moveInstruction);
            return {};
        }
        case A_LITERAL:
        {
            std::string tmp{};
            tmp = genGlobalDest();
            auto* symbol = new Symbol;
            symbol->identifier = tmp;
            symbol->type = new CType;
            symbol->type->typeSpecifier.push_back({
                .token = CHAR
            });
            symbol->string_literal = expr->identifier;
            auto* pointer = new Pointer;
            pointer->setConst();
            symbol->type->declaratorPartList.push_back(pointer);
            globalSyms.push_back(symbol);
            parserState.globalSymbolTable.push_back(symbol);
            auto* gep = new GetElementPtr;
            gep->dest = genTmpDest();
            gep->src = tmp;
            gep->opcode = AVMOpcode::GEP;
            currentBasicBlock->sequenceOfInstructions.push_back(gep);
            return gep->dest;
        }
        case A_IFDECL:
        case A_WHILEBODY:
        case A_FORDECL:
        {
            return "NBB";
        }
        default:
        {
            if (ASTopIsCMPOp(expr->op))
            {
                auto* comparisonInstruction = new ComparisonInstruction;
                comparisonInstruction->compareCode = toCMPCode(expr->op);
                comparisonInstruction->op1 = genCode(expr->left);
                comparisonInstruction->op2 = genCode(expr->right);
                comparisonInstruction->opcode = AVMOpcode::CMP;
                comparisonInstruction->dest = genTmpDest();
                auto* tempSymbol = new Symbol;
                tempSymbol->type = new CType;
                tempSymbol->identifier = comparisonInstruction->dest;
                tempSymbol->type->typeSpecifier.push_back({.token = INTEGER, .lexeme = "int", .lineNumber = 0});
                parserState.globalSymbolTable.push_back(tempSymbol);
                currentFunction->variablesInFunction.push_back(tempSymbol);
                currentBasicBlock->sequenceOfInstructions.push_back(comparisonInstruction);
                return comparisonInstruction->dest;
            }
            if (ASTopIsBinOp(expr->op))
            {
                auto* arithmeticInstruction = new ArithmeticInstruction;
                arithmeticInstruction->src1 = genCode(expr->left);
                arithmeticInstruction->src2 = genCode(expr->right);
                arithmeticInstruction->opcode = toAVM(expr->op);
                arithmeticInstruction->dest = genTmpDest();
                /*if (arithmeticInstruction->src1.at(0) == '#')
                {
                    std::string temp;
                    temp = arithmeticInstruction->src1;
                    arithmeticInstruction->src1 = arithmeticInstruction->src2;
                    arithmeticInstruction->src2 = temp;
                }*/
                auto* tempSymbol = new Symbol;
                tempSymbol->type = new CType;
                tempSymbol->identifier = arithmeticInstruction->dest;
                tempSymbol->type->typeSpecifier.push_back({.token = INTEGER, .lexeme = "int", .lineNumber = 0});
                parserState.globalSymbolTable.push_back(tempSymbol);
                currentFunction->variablesInFunction.push_back(tempSymbol);
                currentBasicBlock->sequenceOfInstructions.push_back(arithmeticInstruction);
                return arithmeticInstruction->dest;
            }

        }

    }
    return {};
}


/*
void AVM::ifHandler(ASTNode *ifDecl, ASTNode* continuation) {
    auto* trueBasicBlock = new AVMBasicBlock;
    auto* branchInstruction = new BranchInstruction;
    branchInstruction->dependantComparison = genCode(ifDecl->left);
    branchInstruction->opcode = AVMOpcode::BR;
    branchInstruction->trueTarget = genLabel();
    trueBasicBlock->label = branchInstruction->trueTarget;
    currentBasicBlock->sequenceOfInstructions.push_back(branchInstruction);
    AVMBasicBlock* currentPath = currentBasicBlock;
    currentBasicBlock = trueBasicBlock;
    auto string = genCode(ifDecl->right->left);
    currentFunction->basicBlocksInFunction.push_back(currentBasicBlock);
    if (string == "IF") // this is the result of a nested if statement
    {
        ifHandler(ifDecl->right->left, ifDecl->right->right);
    }
    branchInstruction->falseTarget = genLabel();
    auto* falseBasicBlock = new AVMBasicBlock;
    falseBasicBlock->label = branchInstruction->falseTarget;
    currentBasicBlock = falseBasicBlock;
    if (ifDecl->right->right != nullptr)
    {
        string = genCode(ifDecl->right->right);
        currentFunction->basicBlocksInFunction.push_back(currentBasicBlock);
        if (string == "IF") // this is the result of a nested if statement
        {
            ifHandler(ifDecl->right->right, nullptr);
        }
    }
    if (continuation == nullptr)
        return;
    auto* continuationBasicBlock = new AVMBasicBlock;
    auto* jmpToContinuationBasicBlock = new BranchInstruction;
    jmpToContinuationBasicBlock->falseTarget = "NULL";
    jmpToContinuationBasicBlock->opcode = AVMOpcode::BR;
    jmpToContinuationBasicBlock->dependantComparison = "#1";
    jmpToContinuationBasicBlock->trueTarget = genLabel();
    continuationBasicBlock->label = jmpToContinuationBasicBlock->trueTarget;
    currentBasicBlock = continuationBasicBlock;
    genCode(continuation);
    currentFunction->basicBlocksInFunction.push_back(continuationBasicBlock);
    trueBasicBlock->sequenceOfInstructions.push_back(jmpToContinuationBasicBlock);
    falseBasicBlock->sequenceOfInstructions.push_back(jmpToContinuationBasicBlock);
}
*/


AVM::AVM(CParse &parserState) : parserState(parserState) {
    for (const auto& i : parserState.currentScope->rst.SymbolHashMap)
    {
        globalSyms.push_back(parserState.globalSymbolTable[i.second]);
    }
}

AVM::~AVM() {
    for (auto i: compilationUnit)
        delete i;
}

std::vector<AVMBasicBlock*> AVM::newBasicBlockHandler(ASTNode *node, ASTNode *nextBasicBlock, bool nested) {
    switch (node->op) {
        case A_IFDECL:
        {
            auto* branchInstruction = new BranchInstruction;
            branchInstruction->dependantComparison = genCode(node->left);
            branchInstruction->opcode = AVMOpcode::BR;
            branchInstruction->trueTarget = genLabel();
            branchInstruction->falseTarget = genLabel();
            currentBasicBlock->sequenceOfInstructions.push_back(branchInstruction);

            // Done with previous basic block
            auto* trueBasicBlock = new AVMBasicBlock;
            auto* falseBasicBlock = new AVMBasicBlock;
            trueBasicBlock->label = branchInstruction->trueTarget;
            falseBasicBlock->label = branchInstruction->falseTarget;
            currentFunction->basicBlocksInFunction.push_back(trueBasicBlock);
            currentFunction->basicBlocksInFunction.push_back(falseBasicBlock);

            currentBasicBlock = trueBasicBlock;
            auto string = genCode(node->right->left->op == A_CS ? node->right->left->left : node->right->left);
            std::vector<AVMBasicBlock*> basicBlocks;
            if (string == "NBB")
            {
                basicBlocks = newBasicBlockHandler(node->right->left->op == A_CS ? node->right->left->left : node->right->left, nullptr, true);
            }
            string = "";

            currentBasicBlock = falseBasicBlock;
            if (node->right->right != nullptr)
                string = genCode(node->right->right->op == A_CS ? node->right->right->left : node->right->right);
            std::vector<AVMBasicBlock*> basicBlocks2;
            if (string == "NBB")
                basicBlocks2 = newBasicBlockHandler(node->right->right->op == A_CS ? node->right->right->left : node->right->right, nullptr, true);

            basicBlocks.insert(basicBlocks.end(), basicBlocks2.begin(), basicBlocks2.end());

            if (nested || (nextBasicBlock == nullptr)) {
                basicBlocks.push_back(trueBasicBlock);
                basicBlocks.push_back(falseBasicBlock);
                return basicBlocks;
            }

            auto* continuation = new AVMBasicBlock;
            continuation->label = genLabel();
            currentBasicBlock = continuation;
            genCode(nextBasicBlock);

            for (auto it : basicBlocks)
            {
                auto* secondBranchInstruction = new BranchInstruction;
                secondBranchInstruction->opcode = AVMOpcode::BR;
                secondBranchInstruction->dependantComparison = "#1";
                secondBranchInstruction->falseTarget = "NULL";
                secondBranchInstruction->trueTarget = continuation->label;
                it->sequenceOfInstructions.push_back(secondBranchInstruction);
            }
            if (trueBasicBlock->sequenceOfInstructions.empty())
            {
                auto* secondBranchInstruction = new BranchInstruction;
                secondBranchInstruction->opcode = AVMOpcode::BR;
                secondBranchInstruction->dependantComparison = "#1";
                secondBranchInstruction->falseTarget = "NULL";
                secondBranchInstruction->trueTarget = continuation->label;
                trueBasicBlock->sequenceOfInstructions.push_back(secondBranchInstruction);
            }
            else if (trueBasicBlock->sequenceOfInstructions.at(trueBasicBlock->sequenceOfInstructions.size()-1)->getInstructionType() != AVMInstructionType::BRANCH) {
                auto* secondBranchInstruction = new BranchInstruction;
                secondBranchInstruction->opcode = AVMOpcode::BR;
                secondBranchInstruction->dependantComparison = "#1";
                secondBranchInstruction->falseTarget = "NULL";
                secondBranchInstruction->trueTarget = continuation->label;
                trueBasicBlock->sequenceOfInstructions.push_back(secondBranchInstruction);
            }
            if (falseBasicBlock->sequenceOfInstructions.empty())
            {
                auto* secondBranchInstruction = new BranchInstruction;
                secondBranchInstruction->opcode = AVMOpcode::BR;
                secondBranchInstruction->dependantComparison = "#1";
                secondBranchInstruction->falseTarget = "NULL";
                secondBranchInstruction->trueTarget = continuation->label;
                falseBasicBlock->sequenceOfInstructions.push_back(secondBranchInstruction);
            }
            else if (falseBasicBlock->sequenceOfInstructions.at(falseBasicBlock->sequenceOfInstructions.size()-1)->getInstructionType() != AVMInstructionType::BRANCH) {
                auto* secondBranchInstruction = new BranchInstruction;
                secondBranchInstruction->opcode = AVMOpcode::BR;
                secondBranchInstruction->dependantComparison = "#1";
                secondBranchInstruction->falseTarget = "NULL";
                secondBranchInstruction->trueTarget = continuation->label;
                falseBasicBlock->sequenceOfInstructions.push_back(secondBranchInstruction);
            }
            currentFunction->basicBlocksInFunction.push_back(continuation);
            return {};
        }
        default:
        {
            return {};
        }
    }
}

bool isPowerOfTwo(u64 val)
{
    return std::__popcount(val) == 1;
}
/*
 * void avmOptimiseFunction
 *
 * This function performs all available optimisations on AVM IR, on a per-function basis
 * */
void AVM::avmOptimiseFunction(AVMFunction* function) {
    for (auto it : function->basicBlocksInFunction)
    {
        optMulToShift(it);
        optDivToShift(it);
        optFoldConstants(it);
    }
    //optPropagateConstants(function);
    //for (auto it : function->basicBlocksInFunction)
    //{
        //optMulToShift(it);
        //optDivToShift(it);
        //optFoldConstants(it);
    //}
}
/*
 * Convert multiplications where the multiplicand is a power of two into a shift,
 * performed on a per-basic block basis.
 * This function uses the commutative property of multiplication to reorder
 * cases where the multiplier is a power of two and turn that into a multiplicand
 * */
void AVM::optMulToShift(AVMBasicBlock *basicBlock) {
    std::vector<ArithmeticInstruction*> multiplies;
    std::vector<u64> indexes;

    for (auto x = 0; x < basicBlock->sequenceOfInstructions.size(); x++) // Loop through basic block and find any relevant multiplies
    {
        auto* it = basicBlock->sequenceOfInstructions.at(x);
        if (it->getInstructionType() == AVMInstructionType::ARITHMETIC)
        {
            auto* instruction = dynamic_cast<ArithmeticInstruction*>(it);
            if (it->opcode == AVMOpcode::MUL) {
                multiplies.push_back(instruction);
                indexes.push_back(x);
            }
        }
    }

    for (auto x = 0; x < multiplies.size(); x++)
    {
        auto* it = multiplies.at(x);
        bool multiplicandIsConstant = false;
        u64 multiplicandValue = 0;
        u64 multiplicandPos = 0;
        bool multiplierIsConstant = false;
        u64 multiplierValue = 0;
        u64 multiplierPos = 0;

        if (it->src2.at(0) == '#')
        {
            multiplierIsConstant = true;
            std::string tmp;
            tmp.append(it->src2);
            tmp.erase(0, 1);
            multiplierValue = std::stoull(tmp);
        }
        if (it->src1.at(0) == '#')
        {
            multiplicandIsConstant = true;
            std::string tmp;
            tmp.append(it->src2);
            tmp.erase(0, 1);
            multiplicandValue = std::stoull(tmp);
        }

        if (multiplierIsConstant && isPowerOfTwo(multiplierValue))
        {
            std::string tmp;
            tmp.append("#");
            tmp.append(std::to_string((u64)std::trunc(std::log2(multiplierValue))));
            it->src2 = tmp;
            it->opcode = AVMOpcode::SLL;
        } else if (multiplicandIsConstant && isPowerOfTwo(multiplicandValue))
        {
            it->src1 = it->src2;
            std::string tmp;
            tmp.append("#");
            tmp.append(std::to_string((u64)std::trunc(std::log2(multiplicandValue))));
            it->src2 = tmp;
            it->opcode = AVMOpcode::SLL;
        }
    }
}
/*
 * In cases where the divisor is a power of two, converts divisions into right shifts
 * */
void AVM::optDivToShift(AVMBasicBlock* basicBlock)
{
    std::vector<ArithmeticInstruction*> divides;
    std::vector<u64> indexes;

    for (auto x = 0; x < basicBlock->sequenceOfInstructions.size(); x++) // Loop through basic block and find any relevant multiplies
    {
        auto* it = basicBlock->sequenceOfInstructions.at(x);
        if (it->getInstructionType() == AVMInstructionType::ARITHMETIC)
        {
            auto* instruction = dynamic_cast<ArithmeticInstruction*>(it);
            if (it->opcode == AVMOpcode::DIV) {
                divides.push_back(instruction);
                indexes.push_back(x);
            }
        }
    }

    for (auto x = 0; x < divides.size(); x++)
    {
        auto* it = divides.at(x);
        bool divisorIsConstant = false;
        u64 divisorValue = 0;

        if (it->src2.at(0) == '#')
        {
            divisorIsConstant = true;
            std::string tmp;
            tmp.append(it->src2);
            tmp.erase(0, 1);
            divisorValue = std::stoull(tmp);
        }
        if (divisorIsConstant && isPowerOfTwo(divisorValue))
        {
            std::string tmp;
            tmp.append("#");
            tmp.append(std::to_string((u64)std::trunc(std::log2(divisorValue))));
            it->src2 = tmp;
            it->opcode = AVMOpcode::ASR;
        }
    }
}

u64 performCalculation(AVMOpcode opcode, u64 operand1, u64 operand2)
{
    switch (opcode) {
        case AVMOpcode::ADD:
        {
            return operand1 + operand2;
        }
        case AVMOpcode::SUB:
        {
            return operand1 - operand2;
        }
        case AVMOpcode::MUL:
        {
            return operand1 * operand2;
        }
        case AVMOpcode::DIV: {
            return operand1 / operand2;
        }
        case AVMOpcode::MOD:
        {
            return operand1 % operand2;
        }
        case AVMOpcode::SLL:
        {
            return operand1 << operand2;
        }
        case AVMOpcode::SLR:
        {
            return operand1 >> operand2;
        }
        case AVMOpcode::ASR: {
            return (signed)operand1 >> operand2;
        }
        case AVMOpcode::AND:
        {
            return operand1 & operand2;
        }
        case AVMOpcode::ORR: {
            return operand1 | operand2;
        }
        case AVMOpcode::XOR: {
            return operand1 ^ operand2;
        }
        default:
        {
            return 0;
        }
    }
}

/*
 * Inspect each arithmetic instruction and eliminate any calculation of constants at runtime
 * */
void AVM::optFoldConstants(AVMBasicBlock* basicBlock)
{
    std::vector<ArithmeticInstruction*> listOfArithmeticInstructions;
    std::vector<u64> indexes;
    for (auto x = 0; x < basicBlock->sequenceOfInstructions.size(); x++) // First gather all arithmetic instructions
    {
        auto it = basicBlock->sequenceOfInstructions.at(x);
        if (it->getInstructionType() == AVMInstructionType::ARITHMETIC)
        {
            listOfArithmeticInstructions.push_back(dynamic_cast<ArithmeticInstruction*>(it));
            indexes.push_back(x);
        }
    }
    // Now we have all of them let's inspect their operands
    for (auto x = 0; x < listOfArithmeticInstructions.size(); x++)
    {
        auto instruction = listOfArithmeticInstructions.at(x);
        if (instruction->src2.at(0) == '#' && instruction->src1.at(0) == '#')
        {
            std::string tmp0 = instruction->src1;
            tmp0.erase(0, 1);
            std::string tmp1 = instruction->src2;
            tmp1.erase(0, 1);

            u64 value = performCalculation(instruction->opcode, std::stoull(tmp0), std::stoull(tmp1));

            auto* moveInstruction = new MoveInstruction;
            moveInstruction->dest = instruction->dest;
            moveInstruction->opcode = AVMOpcode::MV;
            moveInstruction->valueToBeMoved.append("#");
            moveInstruction->valueToBeMoved.append(std::to_string(value));
            delete instruction;
            u64 index = indexes.at(x);
            basicBlock->sequenceOfInstructions.at(index) = moveInstruction;
        }
    }
}


/*
 * Works on two types of instruction currently, arithmetic and move
 *
 * */
void AVM::copyPropagation(AVMFunction* function) {

}
/*
 * All moves with constants are tabulated, then each instruction referencing one of them will be replaced by the constant
 * */
void AVM::optPropagateConstants(AVMFunction* function)
{
    std::vector<std::pair<std::string, std::string>> table;
    for (auto basicBlock : function->basicBlocksInFunction)
    {
        for (auto x = 0; x < basicBlock->sequenceOfInstructions.size(); x++)
        {
            auto instruction = basicBlock->sequenceOfInstructions.at(x);
            if (instruction->getInstructionType() == AVMInstructionType::MV)
            {
                auto move = dynamic_cast<MoveInstruction*>(instruction);
                if (move->valueToBeMoved.at(0) == '#') {
                    table.emplace_back(move->dest, move->valueToBeMoved);
                    basicBlock->sequenceOfInstructions.erase(basicBlock->sequenceOfInstructions.begin() + x);
                }
            }
        }
    }

    // Propogate constants for arithmetics

    for (auto basicBlock : function->basicBlocksInFunction)
    {
        for (auto instruction : basicBlock->sequenceOfInstructions)
        {
            switch (instruction->getInstructionType()) {
                case AVMInstructionType::ARITHMETIC:
                {
                    auto arithmeticInstruction = dynamic_cast<ArithmeticInstruction*>(instruction);
                    for (const auto& pair : table)
                    {
                        if (arithmeticInstruction->src1 == pair.first)
                            arithmeticInstruction->src1 = pair.second;
                        if (arithmeticInstruction->src2 == pair.first)
                            arithmeticInstruction->src2 = pair.second;
                    }
                    break;
                }
                case AVMInstructionType::LOAD:
                    break;
                case AVMInstructionType::STORE:
                    break;
                case AVMInstructionType::GEP:
                    break;
                case AVMInstructionType::CMP:
                    break;
                case AVMInstructionType::BRANCH:
                    break;
                case AVMInstructionType::CALL:
                    break;
                case AVMInstructionType::RET:
                    break;
                case AVMInstructionType::MV: {
                    auto moveInstruction = dynamic_cast<MoveInstruction*>(instruction);
                    for (const auto& pair : table)
                    {
                        if (moveInstruction->valueToBeMoved == pair.first)
                            moveInstruction->valueToBeMoved = pair.second;
                    }
                }
                case AVMInstructionType::ALLOCA:
                    break;
                case AVMInstructionType::END:
                    break;
            }
        }
    }

}