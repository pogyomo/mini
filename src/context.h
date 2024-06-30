#ifndef MINI_CONTEXT_H_
#define MINI_CONTEXT_H_

#include <cstdint>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
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

class VariableEntry;
class StructEntry;
class EnumEntry;
class FunctionEntry;

class SymbolTableEntryVisitor {
public:
    virtual ~SymbolTableEntryVisitor() {}
    virtual void visit(const VariableEntry &entry) = 0;
    virtual void visit(const StructEntry &entry) = 0;
    virtual void visit(const EnumEntry &entry) = 0;
    virtual void visit(const FunctionEntry &entry) = 0;
};

class SymbolTableEntry {
public:
    virtual ~SymbolTableEntry() {}
    virtual void accept(SymbolTableEntryVisitor &visitor) const = 0;
};

class VariableEntry : public SymbolTableEntry {
public:
    VariableEntry(uint64_t offset, const std::shared_ptr<Type> &type,
                  Span var_span)
        : offset_(offset), type_(type), var_span_(var_span) {}
    void accept(SymbolTableEntryVisitor &visitor) const override {
        visitor.visit(*this);
    }
    uint64_t offset() const { return offset_; }
    std::shared_ptr<Type> type() const { return type_; }
    Span var_span() const { return var_span_; }

private:
    uint64_t offset_;
    std::shared_ptr<Type> type_;
    Span var_span_;
};

class StructField {
public:
    StructField(const std::shared_ptr<Type> &type, std::string &&name,
                Span span)
        : type_(type), name_(name), span_(span) {}
    const std::shared_ptr<Type> &type() const { return type_; }
    const std::string &name() const { return name_; }
    Span span() const { return span_; }

private:
    std::shared_ptr<Type> type_;
    std::string name_;
    Span span_;
};

class StructEntry : public SymbolTableEntry {
public:
    StructEntry(std::vector<StructField> &&fields, Span span)
        : fields_(std::move(fields)), span_(span) {}
    void accept(SymbolTableEntryVisitor &visitor) const override {
        visitor.visit(*this);
    }
    const std::vector<StructField> &fields() const { return fields_; }
    Span span() const { return span_; }

private:
    std::vector<StructField> fields_;
    Span span_;
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

class EnumEntry : public SymbolTableEntry {
public:
    EnumEntry(std::vector<EnumField> &&fields, Span span)
        : fields_(std::move(fields)), span_(span) {}
    void accept(SymbolTableEntryVisitor &visitor) const override {
        visitor.visit(*this);
    }
    const std::vector<EnumField> &fields() const { return fields_; }
    Span span() const { return span_; }

private:
    std::vector<EnumField> fields_;
    Span span_;
};

class FunctionEntry : public SymbolTableEntry {
public:
    FunctionEntry(const std::optional<std::shared_ptr<Type>> &ret,
                  std::vector<std::shared_ptr<Type>> &&params, Span span)
        : ret_(ret), params_(std::move(params)), span_(span) {}
    void accept(SymbolTableEntryVisitor &visitor) const override {
        visitor.visit(*this);
    }
    const std::optional<std::shared_ptr<Type>> &ret() const { return ret_; }
    const std::vector<std::shared_ptr<Type>> &params() const { return params_; }
    Span span() const { return span_; }

private:
    std::optional<std::shared_ptr<Type>> ret_;
    std::vector<std::shared_ptr<Type>> params_;
    Span span_;
};

class SymbolTable {
public:
    SymbolTable() : outer_(nullptr) {}
    SymbolTable(const std::shared_ptr<SymbolTable> &outer) : outer_(outer) {}
    const std::shared_ptr<SymbolTable> &outer() const { return outer_; }
    void insert(std::string &&name, std::unique_ptr<SymbolTableEntry> &&entry) {
        map_.emplace(std::make_pair(std::move(name), std::move(entry)));
    }
    // Throw `out_of_range` when no symbol named `name`.
    const std::unique_ptr<SymbolTableEntry> &query(const std::string &name) {
        try {
            return map_.at(name);
        } catch (std::out_of_range &e) {
            if (outer_) {
                return outer_->query(name);
            } else {
                throw std::out_of_range("`query` called from base table");
            }
        }
    }
    bool exists(const std::string &name) {
        try {
            query(name);
            return true;
        } catch (std::out_of_range &e) {
            return false;
        }
    }

private:
    std::shared_ptr<SymbolTable> outer_;
    std::map<std::string, std::unique_ptr<SymbolTableEntry>> map_;
};

class Context {
public:
    Context()
        : symbol_table_(std::make_shared<SymbolTable>()),
          label_count_(0),
          suppress_report_(false) {}
    InputCache &input_cache() { return input_cache_; }
    std::shared_ptr<SymbolTable> &symbol_table() { return symbol_table_; }
    void dive_symbol_table() {
        symbol_table_ = std::make_shared<SymbolTable>(symbol_table_);
    }
    // Throw `runtime_error` when `symbol_table` doesn't have outer table.
    void float_symbol_table() {
        symbol_table_ = symbol_table_->outer();
        if (!symbol_table_) throw std::runtime_error("float from base table");
    }
    bool suppress_report() const { return suppress_report_; }
    void enable_suppress_report() { suppress_report_ = true; }
    void disable_suppress_report() { suppress_report_ = false; }
    // Fetch unique integer to be used in label at codegen.
    int fetch_unique_label_id() { return label_count_++; }

private:
    InputCache input_cache_;
    std::shared_ptr<SymbolTable> symbol_table_;
    int label_count_;
    bool suppress_report_;
};

#endif  // MINI_CONTEXT_H_
