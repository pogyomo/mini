#ifndef MINI_CONTEXT_H_
#define MINI_CONTEXT_H_

#include <fstream>
#include <string>
#include <utility>
#include <vector>

#include "panic.h"

namespace mini {

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
    size_t Cache(std::string &&name, std::vector<std::string> &&lines) {
        size_t id = entries_.size();
        entries_.emplace_back(std::move(name), std::move(lines));
        return id;
    }
    size_t Cache(const std::string &path) {
        std::string line;
        std::vector<std::string> lines;
        std::ifstream ifs(path);
        if (!ifs.is_open()) FatalError("failed to open `{}`", path);
        while (std::getline(ifs, line)) {
            lines.emplace_back(line);
        }
        std::string name = path;
        return Cache(std::move(name), std::move(lines));
    }
    const InputCacheEntry &Fetch(size_t id) const { return entries_.at(id); }

private:
    std::vector<InputCacheEntry> entries_;
};

class Context {
public:
    Context() : should_report_(true) {}
    InputCache &input_cache() { return input_cache_; }
    bool should_report() const { return should_report_; }
    void SuppressReport() { should_report_ = false; }
    void ActivateReport() { should_report_ = true; }

private:
    InputCache input_cache_;
    bool should_report_;
};

};  // namespace mini

#endif  // MINI_CONTEXT_H_
