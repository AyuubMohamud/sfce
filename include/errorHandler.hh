#pragma once

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"
void print_error(const char* string);

void print_error(unsigned long long line, const char* string);

void print_note(unsigned long long line, const char* string);

void print_warning(unsigned long long line, const char* string);

void print_error(unsigned long long line, const char* funcName, const char* type1, const char* type2);
void print_error(unsigned long long line, const char* funcName, const char* type1);