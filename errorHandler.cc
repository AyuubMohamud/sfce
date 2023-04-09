#include <sfce.hh>
#include <errorHandler.hh>
const char* warningTypes[] = {
        "-Wuseless-expression",
        "-Wuninitialised-variable",
        "-Wdivision-by-zero",
        "-Wtype-convert",
        "-Wunused-variable",
        "-Wreturn-value"
};
const char* fullMessages[] = {
        "Useless expression",
        "Unitialised variable",
        "Division by zero is undefined",
        "Type downsizing is an architecture defined behaviour",
        "Variable is unused",
        "Does not return a value on all control paths"
};
const char* errorTypes[] = {
        "Undeclared identifier",
        "Incompatible types",
        "Expected ; after expression"
};
void print_error(unsigned long long line, const char* string)
{
    printf(ANSI_COLOR_RED);
    printf("ERROR: ");
    printf(ANSI_COLOR_RESET);
    printf("%s at line %llu\n", string, line);
}
void print_error(const char* string)
{
    printf(ANSI_COLOR_RED);
    printf("ERROR: ");
    printf(ANSI_COLOR_RESET);
    printf("%s\n", string);
}

void print_note(unsigned long long line, const char* string)
{
    printf(ANSI_COLOR_GREEN);
    printf("note: ");
    printf(ANSI_COLOR_RESET);
    printf("%s at line %llu\n", string, line);
}

void report(int line, const char* message)
{
    printf("%s at %d\n", message, line);
}

void debug_print(const char* message)
{
#ifdef DEBUG
    printf("%s\n", message);
#endif
}

void print_warning(unsigned long long line, const char* string, const char* funcName, i8 errorNum)
{
    printf("In function %s, line %llu\n", funcName, line);
    printf(ANSI_COLOR_MAGENTA);
    printf("Warning:");
    printf(ANSI_COLOR_RESET);
    int x = errorNum > 4 ? 0 : errorNum;
    printf("%s\n", warningTypes[x]);
}

void print_error(unsigned long long line, const char* funcName, const char* type1, const char* type2)
{
    printf(ANSI_COLOR_RED);
    printf("ERROR: ");
    printf(ANSI_COLOR_RESET);
    printf("in %s: file line %llu: ", funcName, line);
    printf("%s and %s are not compatible with each other\n", type1, type2);
}

void print_error(unsigned long long line, const char* funcName, const char* type1)
{
    printf(ANSI_COLOR_RED);
    printf("ERROR: ");
    printf(ANSI_COLOR_RESET);
    printf("in %s: file line %llu: ", funcName, line);
    printf("%s is not compatible with operation\n", type1);
}