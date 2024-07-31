#ifndef MINI_HIR_HIR_H_
#define MINI_HIR_HIR_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "decl.h"
#include "fmt/format.h"
#include "printable.h"

namespace mini {

namespace hir {

// Convert a string into its global symbol.
class StringTable {
public:
    StringTable() : count_(0) {}
    bool SymbolExists(const std::string &s) const {
        return string_to_symbol_.find(s) != string_to_symbol_.end();
    }
    void AddString(std::string &&s) {
        // TODO: Generates unique symbol
        std::string symbol = fmt::format("string_literal_{}", count_++);
        string_to_symbol_.insert(
            std::make_pair(std::move(s), std::move(symbol)));
    }
    const std::string &QuerySymbol(const std::string &s) const {
        if (!SymbolExists(s)) {
            FatalError("");
        } else {
            return string_to_symbol_.at(s);
        }
    }
    const std::map<std::string, std::string> &InnerRepr() const {
        return string_to_symbol_;
    }

private:
    std::map<std::string, std::string> string_to_symbol_;
    uint64_t count_;
};

class Root : public Printable {
public:
    Root(StringTable &&string_table,
         std::vector<std::unique_ptr<hir::Declaration>> &&decls)
        : string_table_(std::move(string_table)), decls_(std::move(decls)) {}
    void Print(PrintableContext &ctx) const override;
    const StringTable &string_table() const { return string_table_; }
    const std::vector<std::unique_ptr<hir::Declaration>> &decls() const {
        return decls_;
    }

private:
    StringTable string_table_;
    std::vector<std::unique_ptr<hir::Declaration>> decls_;
};

}  // namespace hir

}  // namespace mini

#endif  // MINI_HIR_HIR_H_
