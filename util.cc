#include <cparse.hh>


std::unordered_map<CMPCode, std::string> CMPtoString {
        {CMPCode::LT, "lt"},
        {CMPCode::MT, "mt"},
        {CMPCode::LTEQ, "lteq"},
        {CMPCode::MTEQ, "mteq"},
        {CMPCode::EQ, "eq"},
        {CMPCode::NEQ, "neq"}
};
std::unordered_map<AVMOpcode, std::string> OpcodeToString {
        {AVMOpcode::ADD, "add"},
        {AVMOpcode::SUB, "sub"},
        {AVMOpcode::MUL, "mul"},
        {AVMOpcode::DIV, "div"},
        {AVMOpcode::MOD, "mod"},
        {AVMOpcode::SLL, "sll"},
        {AVMOpcode::ASR, "asr"},
        {AVMOpcode::SLR, "slr"},
        {AVMOpcode::AND, "and"},
        {AVMOpcode::XOR, "xor"},
        {AVMOpcode::ORR, "orr"}
};
std::unordered_map<ASTop, AVMOpcode> ASTOperationToAVM {
        {A_ADD, AVMOpcode::ADD},
        {A_SUB, AVMOpcode::SUB},
        {A_MULT, AVMOpcode::MUL},
        {A_DIV, AVMOpcode::DIV},
        {A_MODULO, AVMOpcode::MOD},
        {A_SLL, AVMOpcode::SLL},
        {A_ASR, AVMOpcode::ASR},
        {A_SLR, AVMOpcode::SLR},
        {A_AND, AVMOpcode::AND},
        {A_XOR, AVMOpcode::XOR},
        {A_OR, AVMOpcode::ORR}
};

std::string mapOptoString(AVMOpcode op) {
    auto value = OpcodeToString.find(op);
    if (value == OpcodeToString.end())
        return {};
    return value->second;
}
std::string mapConditionCodetoString(CMPCode code) {
    auto value = CMPtoString.find(code);
    if (value == CMPtoString.end())
        return {};
    return value->second;
}

AVMOpcode toAVM(ASTop op)
{
    auto value = ASTOperationToAVM.find(op);
    if (value == ASTOperationToAVM.end())
        return AVMOpcode::NOP;
    return value->second;
}
CMPCode toCMPCode(ASTop op)
{
    switch (op) {
        case A_MT: {
            return CMPCode::MT;
        }
        case A_LT: {
            return CMPCode::LT;
        }
        case A_EQ: {
            return CMPCode::EQ;
        }
        case A_NEQ: {
            return CMPCode::NEQ;
        }
        case A_MTEQ: {
            return CMPCode::MTEQ;
        }
        case A_LTEQ: {
            return CMPCode::LTEQ;
        }
    }
    return CMPCode::NC;
}
bool ASTopIsBinOpAVM(ASTop op) {
    return (
            op == A_MULT
            || op == A_AND
            || op == A_LAND
            || op == A_LOR
            || op == A_DIV
            || op == A_MODULO
            || op == A_ADD
            || op == A_SUB
            || op == A_SLL
            || op == A_ASR
            || op == A_SLR
            || op == A_OR
            || op == A_XOR
    );
}