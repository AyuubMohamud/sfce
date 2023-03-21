#include <cctype>
#include <cstdio>
#include <unordered_map>
#include <vector>
#include <lexer.hh>
#include <sfce.hh>
#include <errorHandler.hh>

Lexer::Lexer(const char* filename)
{
    m_filename = filename;
    file.open(m_filename);
    tokenisedInput = new LexerResult;
}

Lexer::~Lexer()
{
    file.close();
    delete tokenisedInput;
}

SBCCCode Lexer::secondPass()
{
    for (auto& i : tokenisedInput->TokenisedInput)
    {
        std::unordered_map<std::string, TokenType>::const_iterator value = hashMap.find(i.lexeme);
        if (value != hashMap.end())
        {
            i.token = value->second;
        }
    }
    return OK;
}

LexerResult* Lexer::lexer()
{
    if (file.fail())
    {
        print_error("FILE NOT PRESENT!");
        tokenisedInput->returnCode = FileNotPresent;
        return tokenisedInput;
    }

    while (!file.eof())
    {
        i8 c = advance();
        switch (c) {
            case EOF:
                //addToken(END_OF_FILE, "EOF");
                break;
            case '{':
                addToken(OPEN_BRACE, "{");
                break;
            case '}':
                addToken(CLOSE_BRACE, "}");
                break;
            case '(':
                addToken(OPEN_PARENTHESES, "(");
                break;
            case ')':
                addToken(CLOSE_PARENTHESES, ")");
                break;
            case '\n':
                line++;
                break;
            case ';':
                addToken(SEMICOLON, ";");
                break;
            case ' ':
                break;
            case '\t':
                break;
            case '/':
                backslash();
                break;
            case '"':
                stringLiterals();
                break;
            case '[':
                addToken(OPENBRACKETS, "[");
                break;
            case ']':
                addToken(CLOSEBRACKETS, "]");
                break;
            case '.':
                if (!std::isdigit(peek()))
                {
                    addToken(DOT, ".");
                }
                else {
                    file.unget();
                    numberLiterals();
                }
                break;
            case '?':
                addToken(QUESTION, "?");
                break;
            case ',':
                addToken(COMMA, ",");
            default:
            {
                if (std::isdigit(c) || (c == '.' && std::isdigit(peek())))
                {
                    file.unget();
                    numberLiterals();
                }
                else if (std::isalpha(c))
                {
                    file.unget();
                    identifiers();
                }
                else
                {
                    file.unget();
                    compoundExpressionHandler();
                }
                break;
            }
        }
    }
    addToken(END, "");
    secondPass();
    return tokenisedInput;
}

SBCCCode Lexer::compoundExpressionHandler()
{
    char c = advance();
    switch (c) {
        case '=':
        {
            if (peek() == '=')
            {
                advance();
                addToken(EQUAL, "==");
            }
            else
            {
                addToken(ASSIGNMENT, "=");
            }
            break;
        }
        case '!':
        {
            if (peek() == '=')
            {
                advance();
                addToken(NOTEQUAL, "!=");
            }
            else
            {
                addToken(NEGATE, "!");
            }
            break;
        }
        case '<':
        {
            if (peek() == '=')
            {
                advance();
                addToken(LESSTHANOREQUALTO, "<=");
            }
            else if (peek() == '<')
            {
                advance();
                if (peek() == '=')
                {
                    advance();
                    addToken(COMPOUNDLSL, "<<=");
                }
                else
                {
                    addToken(LSL, "<<");
                }
            }
            else
            {
                addToken(LESSTHAN, "<");
            }
            break;
        }
        case '>':
        {
            if (peek() == '=')
            {
                advance();
                addToken(MORETHANOREQUALTO, ">=");
            }
            else if (peek() == '>')
            {
                advance();
                if (peek() == '=')
                {
                    advance();
                    addToken(COMPOUNDLSR, ">>=");
                }
                else
                {
                    addToken(LSR, ">>=");
                }
            }
            else
            {
                addToken(MORETHAN, ">");
            }
            break;
        }
        case '&':
        {
            if (peek() == '&')
            {
                advance();
                addToken(LOGICALAND, "&&");
            }
            else if (peek() == '=')
            {
                advance();
                addToken(COMPOUNDAND, "&=");
            }
            else
            {
                addToken(AMPERSAND, "&");
            }
            break;
        }
        case '|':
        {
            if (peek() == '|')
            {
                advance();
                addToken(LOGICALORR, "||");
            }
            else if (peek() == '=')
            {
                advance();
                addToken(COMPOUNDORR, "|=");
            }
            else
            {
                addToken(BITWISEORR, "|");
            }
            break;
        }
        case '+':
        {
            if (peek() == '+')
            {
                advance();
                addToken(INCREMENT, "++");
            }
            else if (peek() == '=')
            {
                advance();
                addToken(COMPOUNDADD, "+=");
            }
            else
            {
                addToken(ADD, "+");
            }
            break;
        }
        case '-':
        {
            if (peek() == '-')
            {
                advance();
                addToken(INCREMENT, "--");
            }
            else if (peek() == '=')
            {
                advance();
                addToken(COMPOUNDSUB, "-=");
            }
            else if (peek() == '>')
            {
                advance();
                addToken(POINTEREF, "->");
            }
            else
            {
                addToken(MINUS, "-");
            }
            break;
        }
        case '*':
        {
            if (peek() == '=')
            {
                advance();
                addToken(COMPOUNDMULT, "*=");
            }
            else
            {
                addToken(STAR, "*");
            }
            break;
        }
        case '%':
        {
            if (peek() == '=')
            {
                advance();
                addToken(COMPOUNDMOD, "%=");
            }
            else
            {
                addToken(MODULO, "%");
            }
        }
        case '~':
        {
            addToken(BITWISENOT, "~");
        }
        case '^':
        {
            if (peek() == '=')
            {
                advance();
                addToken(COMPOUNDXOR, "^=");
            }
            else
            {
                addToken(BITWISEXOR, "^");
            }
        }

    }
    return OK;
}

SBCCCode Lexer::identifiers()
{
    std::string literal;
    while (std::isalnum(peek()))
    {
        literal.push_back(advance());
    }
    if (file.eof())
    {
        print_error("Sudden EOF whilst determining identifier?");
        return GeneralError;
    }
    addToken(IDENTIFIER, literal);
    return OK;
}

SBCCCode Lexer::numberLiterals()
{
    std::string literal;
    bool floatingPointLiteral = false;

    while (std::isdigit(peek()) || peek() == '.')
    {
        if (peek() == '.' && !floatingPointLiteral)
        {
            floatingPointLiteral = true;
        }
        else if (peek() == '.' && floatingPointLiteral)
        {
            print_error("Multiple decimal points whilst processing floating point number");
            floatingPointLiteral = false;
            return GeneralError;
        }
        literal.push_back(advance());
    }
    if (file.eof()) {
        print_error("EOF on a numerical literal?");
    }
    else if (floatingPointLiteral)
    {
        addToken(FP_LITERAL, literal);
    }
    else {
        addToken(INTEGER_LITERAL, literal);
    }
    return OK;
}


SBCCCode Lexer::backslash()
{
    if (peek() == '/')
    {
        while (peek() != '\n')
        {
            if (file.eof())
            {
                print_error("Undetermined Comment? We have read to the end of the file and yet we cannot determine the comment made");
                return GeneralError;
            }
            advance();
        }
    }
    else if (peek() == '*')
    {
        char c;
        while (peek() != '/') {
            if (file.eof())
            {
                print_error("Undetermined Comment? We have read to the end of the file and yet we cannot determine the comment made");
                return GeneralError;
            }
            c = advance();
            line = line + (c == '\n' ? 1 : 0);
        }
        advance();
    }
    else if (peek() == '=')
    {
        addToken(COMPOUNDDIV, "/=");
    }
    else
    {
        addToken(BACKSLASH, "/");
    }
    return OK;
}

SBCCCode Lexer::stringLiterals()
{
    std::string literal;
    while (peek() != '"')
    {
        if (file.eof())
        {
            print_error("Undetermined string literal.");
            return GeneralError;
        }
        literal.push_back(advance());
    }
    advance();
    addToken(STRING_LITERAL, literal);
    return OK;
}

inline char Lexer::advance()
{
    return file.get();
}

inline char Lexer::peek()
{
    return file.peek();
}

void Lexer::addToken(TokenType token, std::string lexeme)
{
    Token newToken;
    newToken.token = token;
    newToken.lexeme = lexeme;
    newToken.lineNumber = line;
    tokenisedInput->TokenisedInput.push_back(newToken);
}