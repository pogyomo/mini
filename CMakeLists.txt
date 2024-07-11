cmake_minimum_required(VERSION 3.1)
project(mini CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
    src/ast/stmt.cc
    src/ast/type.cc
    src/hirgen/hirgen.cc
    src/hirgen/item.cc
    src/lexer.cc
    src/main.cc
    src/parser.cc
    src/report.cc
    src/token.cc
)

add_subdirectory(fmt)
target_link_libraries(${PROJECT_NAME} fmt::fmt)
