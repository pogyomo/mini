#ifndef MINI_CONTEXT_H_
#define MINI_CONTEXT_H_

#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ast/type.h"
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

class StructField {
public:
    StructField(std::shared_ptr<Type> type, std::string &&name, Span span)
        : type_(type), name_(name), span_(span) {}
    const std::shared_ptr<Type> &type() const { return type_; }
    const std::string &name() const { return name_; }

private:
    std::shared_ptr<Type> type_;
    std::string name_;
    Span span_;
};

class StructTableEntry {
public:
    StructTableEntry(std::vector<StructField> &&fields)
        : fields_(std::move(fields)) {}
    const std::vector<StructField> &fields() const { return fields_; }

private:
    std::vector<StructField> fields_;
};

class StructTable {
public:
    void insert(std::string &&name, StructTableEntry &&entry) {
        map_.insert({name, entry});
    }

    // Throw `out_of_range` when no struct exists associated with `name`.
    const StructTableEntry &query(const std::string &name) {
        return map_.at(name);
    }

private:
    std::map<std::string, StructTableEntry> map_;
};

class EnumField {
public:
    EnumField(std::string &&name, uint64_t value, Span span)
        : name_(name), value_(value), span_(span) {}
    const std::string &name() const { return name_; }
    uint64_t value() const { return value_; }
    Span span() const { return span_; }

private:
    std::string name_;
    uint64_t value_;
    Span span_;
};

class EnumTableEntry {
public:
    EnumTableEntry(std::vector<EnumField> &&fields)
        : fields_(std::move(fields)) {}
    const std::vector<EnumField> &fields() const { return fields_; }

private:
    std::vector<EnumField> fields_;
};

class EnumTable {
public:
    void insert(std::string &&name, EnumTableEntry &&entry) {
        map_.insert({name, entry});
    }

    // Throw `out_of_range` when no enum exists associated with `name`.
    const EnumTableEntry &query(const std::string &name) {
        return map_.at(name);
    }

private:
    std::map<std::string, EnumTableEntry> map_;
};

class SymbolTableEntry {
public:
    SymbolTableEntry(std::shared_ptr<Type> type, Span var_span)
        : type_(type), var_span_(var_span) {}
    std::shared_ptr<Type> type() const { return type_; }
    Span var_span() const { return var_span_; }

private:
    std::shared_ptr<Type> type_;
    Span var_span_;
};

class SymbolTable {
public:
    void insert(std::string &&name, SymbolTableEntry &&entry) {
        map_.insert({name, entry});
    }

    // Throw `out_of_range` when no symbol named `name`.
    const SymbolTableEntry &query(const std::string &name) {
        return map_.at(name);
    }

private:
    std::map<std::string, SymbolTableEntry> map_;
};

class Context {
public:
    InputCache &input_cache() { return input_cache_; }
    StructTable &struct_table() { return struct_table_; }
    EnumTable &enum_table() { return enum_table_; }
    SymbolTable &symbol_table() { return symbol_table_; }
    bool suppress_report() const { return suppress_report_; }
    void enable_suppress_report() { suppress_report_ = true; }
    void disable_suppress_report() { suppress_report_ = false; }

private:
    InputCache input_cache_;
    StructTable struct_table_;
    EnumTable enum_table_;
    SymbolTable symbol_table_;
    bool suppress_report_;
};

#endif  // MINI_CONTEXT_H_
