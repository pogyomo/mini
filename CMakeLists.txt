cmake_minimum_required(VERSION 3.1)
project(mini CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
    src/ast/stmt.cc
    src/ast/type.cc
    src/codegen/asm.cc
    src/codegen/codegen.cc
    src/codegen/context.cc
    src/codegen/decl.cc
    src/codegen/expr.cc
    src/codegen/stmt.cc
    src/codegen/type.cc
    src/eval.cc
    src/hir/decl.cc
    src/hir/expr.cc
    src/hir/stmt.cc
    src/hir/type.cc
    src/hirgen/context.cc
    src/hirgen/decl.cc
    src/hirgen/expr.cc
    src/hirgen/hirgen.cc
    src/hirgen/item.cc
    src/hirgen/stmt.cc
    src/hirgen/type.cc
    src/lexer.cc
    src/main.cc
    src/parser/decl.cc
    src/parser/expr.cc
    src/parser/parser.cc
    src/parser/stmt.cc
    src/parser/type.cc
    src/parser/utils.cc
    src/report.cc
    src/token.cc
)

target_compile_options(${PROJECT_NAME} PUBLIC -O3 -Wall -Wextra)
# Uncomment below when use gdb
# target_compile_options(${PROJECT_NAME} PUBLIC -O0 -g)

add_subdirectory(fmt)
target_link_libraries(${PROJECT_NAME} fmt::fmt)
