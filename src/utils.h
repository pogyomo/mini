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
        } else if (c == '\\') {
            oss << "\\\\";
        } else if (c == '\0') {
            oss << "\\0";
        } else {
            oss << c;
        }
    }
    return oss.str();
}

inline std::string EscapeCharContent(char content) {
    if (content == '\a') {
        return "\\a";
    } else if (content == '\b') {
        return "\\b";
    } else if (content == '\f') {
        return "\\f";
    } else if (content == '\n') {
        return "\\n";
    } else if (content == '\r') {
        return "\\r";
    } else if (content == '\t') {
        return "\\t";
    } else if (content == '\v') {
        return "\\v";
    } else if (content == '\'') {
        return "\\'";
    } else if (content == '\\') {
        return "\\\\";
    } else if (content == '\0') {
        return "\\0";
    } else {
        return std::string(1, content);
    }
}

}  // namespace mini

#endif  // MINI_UTILS_H_
