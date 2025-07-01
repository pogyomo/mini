#ifndef MINI_UTILS_H_
#define MINI_UTILS_H_

#include <sstream>
#include <string>

namespace mini {

inline std::string EscapeStringContent(const std::string &content) {
    std::stringstream oss;
    for (auto c : content) {
        if (c == '\a') {
            oss << "\\a";
        } else if (c == '\b') {
            oss << "\\b";
        } else if (c == '\f') {
            oss << "\\f";
        } else if (c == '\n') {
            oss << "\\n";
        } else if (c == '\r') {
            oss << "\\r";
        } else if (c == '\t') {
            oss << "\\t";
        } else if (c == '\v') {
            oss << "\\v";
        } else if (c == '"') {
            oss << "\\\"";
        } else if (c == '\0') {
            oss << "\\0";
        } else {
            oss << c;
        }
    }
    return oss.str();
}

}  // namespace mini

#endif  // MINI_UTILS_H_
