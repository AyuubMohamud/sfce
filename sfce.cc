#include <cstdio>
#include <cstring>
#include <errorHandler.hh>
#include <lexer.hh>
#include "SBCCVer.h"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"


void help()
{
    printf(ANSI_COLOR_BLUE "Usage: sbcc [filenames] [target_options] -o [output filename]\n" ANSI_COLOR_RESET);
    printf("target_options:\n");
    printf("-LLVM: Use LLVM as code generator\n");
    printf("-native: Use SBCC's code generator\n");
    printf("-rv64i(m/a/c/f/d): Build for RISC V\n");
}

void version()
{
    printf("SBCC %d.%d: Built by %s version %s\n", SBCC_VERSION_MAJOR, SBCC_VERSION_MINOR, COMPILER, COMPILER_VERSION);
}
int main(int argc, const char** argv)
{
    if (argc < 2)
    {
        print_error("SBCC called with no arguments!");
        help();
        return 1;
    }
    if (!strncmp(argv[1], "version", 7))
    {
        version();
        return 0;
    }
    if (!strncmp(argv[1], "help", 4))
    {
        help();
        return 0;
    }
    Lexer* lexer = new Lexer(argv[1]);

    LexerResult* result = lexer->lexer();

    for (auto& i : result->TokenisedInput)
    {
        printf("%s, %lu\n", i.lexeme.c_str(), i.lineNumber);
    }
    delete lexer;


    return 0;
}