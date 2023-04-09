#include <sfce.hh>
#include <cparse.hh>

/*
 *
 * Traverse AST, ignore A_GLUES
 * genCode whenever we see BinOP, INC, DEC, WHILEDECL, AGEN
 *
 * */

void AVM::AVMByteCodeDriver(FunctionAST* functionToBeTranslated) {
    auto* function = new AVMFunction;
    for (const auto& i : parserState.currentScope->rst.SymbolHashMap)
    {
        function->incomingSymbols.push_back(parserState.globalSymbolTable[i.second]);
    }

}

AVMBasicBlock* AVM::cvtBasicBlockToAVMByteCode(ASTNode* node) {
    auto* basicBlock = new AVMBasicBlock;
    currentBasicBlock = basicBlock;
    genCode(node);
    return basicBlock;
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
            genCode(expr->left);
            genCode(expr->right);
            // Since its glue they are not connected, simply ignore their values
        }
        case A_END:
        {
            auto* nop = new ArithmeticInstruction;
            nop->dest = "END";
            currentBasicBlock->sequenceOfInstructions.push_back(nop);
            return {};
        }
        case A_CS:
        {
            genCode(expr->left);
        }
        default:
        {
            if (ASTopIsBinOp(expr->op))
            {
                auto* arithmeticInstruction = new ArithmeticInstruction;
                arithmeticInstruction->dest = genTmpDest();
                arithmeticInstruction->src1 = genCode(expr->left);
                arithmeticInstruction->src2 = genCode(expr->right);
                arithmeticInstruction->opcode = toAVM(expr->op);
                currentBasicBlock->sequenceOfInstructions.push_back(arithmeticInstruction);
            }
        }

    }
    return {};
}

AVM::AVM(CParse &parserState) : parserState(parserState) {

}



