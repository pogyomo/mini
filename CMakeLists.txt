cmake_minimum_required(VERSION 3.1)
project(mini CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
    src/ast/stmt.cc
    src/lexer.cc
    src/main.cc
    src/report.cc
)
