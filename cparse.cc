#include "cparse.hh"

bool isTypeSpecifier(const Token& token)
{
    if (token.token == INTEGER || token.token == CHAR || token.token == SHORT)
    {
        return true;
    }
    return false;
}
CParse::CParse(std::vector<Token>* input)
{
    tokens = input;
}

bool CParse::parse()
{
    while (tokens->at(cursor).token != END_OF_FILE)
    {
        auto* newSymbol = new Symbol;
        newSymbol->type = new CType;
        declarationSpecifiers(newSymbol->type);
        declarator();
    }
    return false;
}

bool CParse::declarationSpecifiers(CType* cType)
{

    return false;
}

CParse::~CParse()
{

}

bool CParse::declarator() {
    return false;
}

bool CParse::combinable(CType *cType, Token token)
{
    switch (token.token) {
        case SHORT:
        case CHAR:
        case INTEGER: {
            bool conflictingTypeFound = false;
            for (auto& i: cType->typeSpecifier)
            {
                if (i.token == CHAR || i.token == SHORT || i.token == INTEGER) // Add floating point types as well
                    conflictingTypeFound = true;
            }
            if (conflictingTypeFound) {
                return false;
            }
            else {return true;}
        }
        case STATIC:
        {
        }
        default:
            break;

    }
}
