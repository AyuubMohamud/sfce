#pragma once

#pragma once
#include <cstdio>
#include <cstdint>

#if defined(__GNUC__) && !defined(__llvm__)
#define COMPILER_VERSION __GNUC__ SPACE __GNUC_MINOR__ SPACE __GNUC_PATCHLEVEL__
#define COMPILER "GCC"
#elif defined(__llvm__)
#define COMPILER_VERSION __clang_version__
#define COMPILER "Clang/LLVM"
#elif defined(_MSC_VER)
#define COMPILER_VERSION _MSC_VER
#define COMPILER "Microsoft Visual C++"
#endif

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

enum SBCCCode {
    OK,
    GeneralError,
    FileNotPresent,
    SyntaxError,
    NotSupported
};

enum ABI
{
    UNIX,
    WINDOWS,
    SYSTEMV,
    DARWIN
};