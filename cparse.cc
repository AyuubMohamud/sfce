#include "cparse.hh"


CParse::CParse(std::vector<Token>* input)
{
    tokens = input;
}

bool CParse::parse()
{
    while (tokens->at(cursor).token != END_OF_FILE)
    {
        declarationSpecifiers();

    }
    return false;
}

bool CParse::declarationSpecifiers()
{

    return false;
}

CParse::~CParse()
{

}