#pragma once

#include <fstream>
#include "sfce.hh"
#include <vector>
#include <unordered_map>
enum TokenType
{
    // KEYWORD
    INTEGER,
    UNSIGNED,
    RETURN,
    CONST,
    TYPEDEF,
    FLOAT,
    DOUBLE,
    CHAR,
    LONG,
    REGISTER,
    AUTO,
    CASE,
    BREAK,
    CONTINUE,
    DEFAULT,
    DO,
    ELSE,
    ENUM,
    EXTERN,
    FOR,
    GOTO,
    IF,
    INLINE,
    RESTRICT,
    SHORT,
    SIGNED,
    SIZEOF,
    STATIC,
    STRUCT,
    SWITCH,
    UNION,
    VOID,
    VOLATILE,
    WHILE,
    BOOL,
    COMPLEX,
    IMAGINERY,
    // Identifiers
    IDENTIFIER,
    // Literals
    STRING_LITERAL,
    INTEGER_LITERAL,
    FP_LITERAL,
    // Symbols
    OPEN_PARENTHESES,
    CLOSE_PARENTHESES,
    SEMICOLON,
    OPEN_BRACE,
    CLOSE_BRACE,

    BACKSLASH,
    STAR,
    NEGATE,
    LESSTHANOREQUALTO,
    MORETHANOREQUALTO,
    MORETHAN,
    LESSTHAN,
    EQUAL,
    NOTEQUAL,
    LOGICALORR,
    LOGICALAND,
    AMPERSAND,
    ASSIGNMENT,
    ADD,
    MINUS,
    BITWISENOT,
    BITWISEORR,
    BITWISEXOR,
    INCREMENT,
    DECREMENT,
    LSL,
    LSR,
    COMPOUNDADD,
    COMPOUNDSUB,
    COMPOUNDMULT,
    COMPOUNDDIV,
    COMPOUNDORR,
    COMPOUNDXOR,
    COMPOUNDAND,
    COMPOUNDMOD,
    COMPOUNDLSL,
    COMPOUNDLSR,
    MODULO,
    DOT,
    POINTEREF,
    QUESTION,
    COLON,
    OPENBRACKETS,
    CLOSEBRACKETS,
    COMMA,
    END
};
class Token
{
public:
    TokenType token;
    std::string lexeme;
    u64 lineNumber;
};

struct LexerResult
{
    std::vector<Token>* TokenisedInput;
    SBCCCode returnCode;
};

class Lexer
{
public:
    explicit Lexer(const char* filename);
    ~Lexer();
    LexerResult* lexer();

private:
    const char* m_filename;
    std::ifstream file;
    char advance();
    char peek();
    void addToken(TokenType token, std::string lexeme);
    u64 line = 0;
    LexerResult* tokenisedInput = nullptr;
    SBCCCode identifiers();
    SBCCCode numberLiterals();
    SBCCCode stringLiterals();
    SBCCCode backslash();
    SBCCCode compoundExpressionHandler();
    SBCCCode secondPass();
    std::unordered_map<std::string, TokenType> hashMap = {
            {"int", INTEGER},
            {"return", RETURN},
            {"const", CONST},
            {"auto", AUTO},
            {"break", BREAK},
            {"case", CASE},
            {"char", CHAR},
            {"const", CONST},
            {"continue", CONTINUE},
            {"default", DEFAULT},
            {"do", DO},
            {"double", DOUBLE},
            {"else", ELSE},
            {"enum", ENUM},
            {"extern", EXTERN},
            {"float", FLOAT},
            {"for", FOR},
            {"goto", GOTO},
            {"if", IF},
            {"inline", INLINE},
            {"long", LONG},
            {"register", REGISTER},
            {"restrict", RESTRICT},
            {"short", SHORT},
            {"signed", SIGNED},
            {"sizeof", SIZEOF},
            {"static", STATIC},
            {"struct", STRUCT},
            {"switch", SWITCH},
            {"typedef", TYPEDEF},
            {"union", UNION},
            {"unsigned", UNSIGNED},
            {"void", VOID},
            {"volatile", VOLATILE},
            {"while", WHILE},
            {"_Complex", COMPLEX},
            {"_Bool", BOOL},
            {"_Imaginery", IMAGINERY}
    };
};
