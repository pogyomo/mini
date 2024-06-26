#ifndef MINI_PANIC_H_
#define MINI_PANIC_H_

#include <string>

template <typename... Args>
void fatal_error(const std::string &fmt, Args... args) {
    std::fprintf(stderr, "\e[31mfatal error: \e[0m");
    std::fprintf(stderr, fmt.c_str(), args...);
    std::fprintf(stderr, "\n");
    std::exit(EXIT_FAILURE);
}

#endif  // MINI_PANIC_H_
