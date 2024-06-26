#ifndef MINI_CONTEXT_H_
#define MINI_CONTEXT_H_

#include <fstream>
#include <string>
#include <vector>

#include "panic.h"

class InputCacheEntry {
public:
    InputCacheEntry(std::string &&name, std::vector<std::string> &&lines)
        : name_(std::move(name)), lines_(std::move(lines)) {}
    inline const std::string &name() const { return name_; }
    inline const std::vector<std::string> &lines() const { return lines_; }

private:
    const std::string name_;
    const std::vector<std::string> lines_;
};

class InputCache {
public:
    InputCache() : entries_() {}
    size_t cache(std::string &&name, std::vector<std::string> &&lines) {
        size_t id = entries_.size();
        entries_.emplace_back(std::move(name), std::move(lines));
        return id;
    }
    size_t cache(const std::string &path) {
        std::string line;
        std::vector<std::string> lines;
        std::ifstream ifs(path);
        if (!ifs.is_open()) fatal_error("failed to open `%s`", path.c_str());
        while (std::getline(ifs, line)) {
            lines.emplace_back(line);
        }
        std::string name = path;
        return cache(std::move(name), std::move(lines));
    }
    const InputCacheEntry &fetch(size_t id) const { return entries_.at(id); }

private:
    std::vector<InputCacheEntry> entries_;
};

class Context {
public:
    InputCache &input_cache() { return input_cache_; }
    bool suppress_report() const { return suppress_report_; }
    void enable_suppress_report() { suppress_report_ = true; }
    void disable_suppress_report() { suppress_report_ = false; }

private:
    InputCache input_cache_;
    bool suppress_report_;
};

#endif  // MINI_CONTEXT_H_
