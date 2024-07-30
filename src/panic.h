#ifndef MINI_PANIC_H_
#define MINI_PANIC_H_

#include <cstdlib>
#include <utility>

#include "fmt/base.h"

namespace mini {

template <typename... T>
[[noreturn]] void FatalError(fmt::format_string<T...> fmt, T&&... args) {
    fmt::print(stderr, "\e[31mfatal error: \e[0m");
    fmt::print(stderr, fmt, std::forward<T>(args)...);
    fmt::println(stderr, "");
    std::exit(EXIT_FAILURE);
}

};  // namespace mini

#endif  // MINI_PANIC_H_
