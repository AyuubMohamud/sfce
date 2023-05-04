#include <cstdio>
#include <cstring>
#include <errorHandler.hh>
#include <lexer.hh>
#include <cparse.hh>
#include <codeGen.hh>

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
    if (argc < 4)
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
    if (strncmp(argv[2], "-o", 2) != 0)
    {
        print_error("Output file not specified!");
        return 1;
    }
    bool optimise = false;
    if (argc > 4)
    {
        if (strncmp(argv[4], "-O0", 8) != 0) {
            optimise = true;
        }
    }
    Lexer lexer(argv[1]);
    LexerResult* result = lexer.lexer();
    if (result->returnCode == SBCCCode::FileNotPresent)
    {
        return 1;
    }
/*
    for (auto& i : *result->TokenisedInput)
    {
        printf("%s, %lu\n", i.lexeme.c_str(), i.lineNumber);
    }
    */
    CParse parser(result->TokenisedInput);
    if (!parser.parse()) {print_error("Error whilst parsing!"); return 1;}

    SemanticAnalyser analyser;
    bool success = analyser.startSemanticAnalysis(parser);
    if (success)
    {
        printf("Semantically OK translation unit, proceeding to produce code!\n");
    }
    else {
        return 1;
    }

    AVM abstractVirtualMachine(parser);
    for (auto* i: parser.functions)
    {
        abstractVirtualMachine.AVMByteCodeDriver(i);
    }
    if (optimise) {
        for (auto *i: abstractVirtualMachine.compilationUnit) {
            abstractVirtualMachine.avmOptimiseFunction(i);
        }
    }
    for (auto* i: abstractVirtualMachine.compilationUnit) {
        printf("DECL FUNC %s\n", i->name.c_str());
        for (auto symbol : i->incomingSymbols)
        {
            printf("PARAM %s: %s\n",symbol->identifier.c_str(), symbol->type->typeAsString().c_str());
        }
        for (auto *x: i->basicBlocksInFunction) {
            printf("LABEL: %s\n", x->label.c_str());
            printf("%s", x->print().c_str());
        }
    }
    CodeGenerator codeGenerator(abstractVirtualMachine, argv[3]);
    codeGenerator.startFinalTranslation();


    return 0;
}