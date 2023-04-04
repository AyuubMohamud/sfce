#pragma once
#include "sfce.hh"

/*
Abstract Virtual Machine.

 This is the IR that comes after the abstract syntax tree.

 Instructions are represented with operands that are a mix of in program variables and temporaries.






 * */

enum class AVMOpcode {
    // Arithmetic class
    ADD,
    SUB,
    MUL,
    DIV,
    SLL,
    SLR,
    AND,
    OR,
    XOR,
    // Memory class
    GEP,
    LD,
    ST,
    // Control Flow class
    CMP,
    BR,
    RET,
    CALL,
    // Miscellaneous
    MV
};