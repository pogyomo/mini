#ifndef MINI_PANIC_H_
#define MINI_PANIC_H_

#include <string>

#include "fmt/base.h"

template <typename... Args>
void fatal_error(const std::string &fmt, Args... args) {
    fmt::print(stderr, "\e[31mfatal error: \e[0m");
    fmt::print(stderr, fmt, args...);
    fmt::println(stderr, "");
    std::exit(EXIT_FAILURE);
}

#endif  // MINI_PANIC_H_
