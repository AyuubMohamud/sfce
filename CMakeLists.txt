cmake_minimum_required(VERSION 3.10)
project(sfce VERSION 0.1)
add_executable(sfce lexer.cc sfce.cc sfce.h.in include/errorHandler.hh cparse.cc include/cparse.hh errorHandler.cc semanticChecker.cc AVM.cc util.cc codeGen.cc include/codeGen.hh)
configure_file(sfce.h.in sfce.h)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS_DEBUG "-std=gnu++20 -O0 -g -DDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-std=gnu++20 -Os")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-std=gnu++20 -O3 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-std=gnu++20 -O3")
set(SBCC_INCLUDE include/)

add_compile_options(-std=gnu++20)
include_directories(${SBCC_INCLUDE})