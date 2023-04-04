#include <sfce.hh>
#include <errorHandler.hh>

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
    printf(ANSI_COLOR_MAGENTA);
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