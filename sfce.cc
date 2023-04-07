#include <cstdio>
#include <cstring>
#include <errorHandler.hh>
#include <lexer.hh>
#include <cparse.hh>
#include <semanticChecker.hh>
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"


void help()
{
    printf(ANSI_COLOR_BLUE "Usage: sfce [filenames] [target_options] -o [output filename]\n" ANSI_COLOR_RESET);
}

void version()
{
    printf("SFCE 0.1: Built by %s\n", COMPILER);
}
int main(int argc, const char** argv)
{
    if (argc < 2)
    {
        print_error("SFCE called with no arguments!");
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
    auto* lexer = new Lexer(argv[1]);

    LexerResult* result = lexer->lexer();

    for (auto& i : *result->TokenisedInput)
    {
        printf("%s, %llu\n", i.lexeme.c_str(), i.lineNumber);
    }
    CParse parser(result->TokenisedInput);
    if (!parser.parse()) print_error("Error whilst parsing!");

    //SemanticAnalyser analyser;
    //analyser.startSemanticAnalysis(parser);
    delete lexer;


    return 0;
}