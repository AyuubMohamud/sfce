#include <sfce.hh>
#include <cparse.hh>
#include <errorHandler.hh>
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
    currentFunction = function;
    for (auto* i : prototype->types)
        currentFunction->incomingSymbols.push_back(i);
    std::string label = "entry";
    bool done = false;
    ASTNode* node = functionToBeTranslated->root;
    do {
        currentNode = nullptr;
        auto* basicBlock = new AVMBasicBlock;
        basicBlock->label = label;
        cvtBasicBlockToAVMByteCode(node, basicBlock);
        if (currentNode == nullptr)
        {
            done = true;
        }
        else {
            node = currentNode;
        }
        label = genLabel();
    } while (!done);
    compilationUnit.push_back(function);
}

void AVM::cvtBasicBlockToAVMByteCode(ASTNode* node, AVMBasicBlock* basicBlock) {
    AVMBasicBlock* previous = nullptr;
    previous = currentBasicBlock;
    currentBasicBlock = basicBlock;
    genCode(node);
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
            auto* ifStatements = ifHandler(cNode->left);
            currentFunction->basicBlocksInFunction.push_back(ifStatements->truePath);
            currentFunction->basicBlocksInFunction.push_back(ifStatements->falsePath);
            delete ifStatements;
            break;
        }
        default:
            break;
    }
    currentBasicBlock = previous;
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
            dereference->dest = genTmpDest();
            dereference->addrVar = genCode(expr->left);
            currentBasicBlock->sequenceOfInstructions.push_back(dereference);
            return dereference->dest;
        }
        case A_AGEN:
        {
            auto* addressGeneration = new GetElementPtr;
            addressGeneration->opcode = AVMOpcode::GEP;
            addressGeneration->dest = genTmpDest();
            addressGeneration->src = genCode(expr->left);
            currentBasicBlock->sequenceOfInstructions.push_back(addressGeneration);
            return addressGeneration->dest;
        }
        case A_IDENT:
        {
            return expr->identifier;
        }
        case A_INTLIT:
        {
            auto* movInstruction = new MoveInstruction;
            movInstruction->dest = genTmpDest();
            movInstruction->valueToBeMoved = "#";
            movInstruction->valueToBeMoved.append(expr->identifier);
            movInstruction->opcode = AVMOpcode::MV;
            currentBasicBlock->sequenceOfInstructions.push_back(movInstruction);
            return movInstruction->dest;
        }
        case A_RET:
        {
            auto* retInstruction = new RetInstruction;
            retInstruction->value = genCode(expr->left);
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
            if (expr->left->op == A_IFDECL)
            {
                currentNode = expr;
                return {};
            }
            genCode(expr->left);
            genCode(expr->right);
            // Since its glue they are not connected, simply ignore their values
            return {};
        }
        case A_END:
        {
            auto* nop = new ArithmeticInstruction;
            nop->dest = "END";
            currentBasicBlock->sequenceOfInstructions.push_back(nop);
            return "END";
        }
        case A_CS:
        {
            if (expr->left->op == A_IFDECL)
            {
                currentNode = expr;
                return {};
            }
            genCode(expr->left);
            return {};
        }
        case A_MV:
        {
            auto* moveInstruction = new MoveInstruction;
            moveInstruction->dest = genCode(expr->left);
            moveInstruction->valueToBeMoved = genCode(expr->right);
            moveInstruction->opcode = AVMOpcode::MV;
            currentBasicBlock->sequenceOfInstructions.push_back(moveInstruction);
            return {};
        }
        default:
        {
            if (ASTopIsCMPOp(expr->op))
            {
                auto* comparisonInstruction = new ComparisonInstruction;
                comparisonInstruction->dest = genTmpDest();
                comparisonInstruction->compareCode = toCMPCode(expr->op);
                comparisonInstruction->op1 = genCode(expr->left);
                comparisonInstruction->op2 = genCode(expr->right);
                comparisonInstruction->opcode = AVMOpcode::CMP;
                currentBasicBlock->sequenceOfInstructions.push_back(comparisonInstruction);
                return comparisonInstruction->dest;
            }
            if (ASTopIsBinOp(expr->op))
            {
                auto* arithmeticInstruction = new ArithmeticInstruction;
                arithmeticInstruction->dest = genTmpDest();
                arithmeticInstruction->src1 = genCode(expr->left);
                arithmeticInstruction->src2 = genCode(expr->right);
                arithmeticInstruction->opcode = toAVM(expr->op);
                currentBasicBlock->sequenceOfInstructions.push_back(arithmeticInstruction);
                return arithmeticInstruction->dest;
            }

        }

    }
    return {};
}
AVMBasicBlocksForIFs* AVM::ifHandler(ASTNode *expr) {
    auto* res = new AVMBasicBlocksForIFs;
    auto* truePath = new AVMBasicBlock; res->truePath = truePath;   res->truePath->label = genLabel();
    if (toCMPCode(expr->left->op) == CMPCode::NC)
    {
        auto* branchInstruction = new BranchInstruction;
        branchInstruction->trueTarget = res->truePath->label;
        branchInstruction->falseTarget = "NULL";
        branchInstruction->dependantComparison = "#1";
        cvtBasicBlockToAVMByteCode(expr->right->left, res->truePath);
        return res;
    }
    auto* falsePath = new AVMBasicBlock;
    res->falsePath = falsePath;
    res->falsePath->label = genLabel();
    auto* branchInstruction = new BranchInstruction;
    branchInstruction->trueTarget = res->truePath->label;
    branchInstruction->falseTarget = res->falsePath->label;
    branchInstruction->dependantComparison = genCode(expr->left);
    ASTNode* saveNode = currentNode;
    currentNode = nullptr;
    cvtBasicBlockToAVMByteCode(expr->right->left, res->truePath);
    cvtBasicBlockToAVMByteCode(expr->right->right, res->falsePath);
    currentNode = saveNode;
    return res;
}



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


