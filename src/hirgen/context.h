#ifndef MINI_HIRGEN_CONTEXT_H_
#define MINI_HIRGEN_CONTEXT_H_

#include <map>
#include <memory>
#include <stack>
#include <stdexcept>
#include <string>

#include "../context.h"
#include "fmt/format.h"

namespace mini {

class VariableTranslator {
public:
    VariableTranslator()
        : assoc_table_(std::make_shared<SymbolAssocTable>()),
          id_tracker_({0}),
          current_scope_id_(0) {}
    bool translatable(const std::string &name) {
        return assoc_table_->exists(name);
    }
    const std::string &regvar(const std::string &name) {
        auto assoc = fmt::format("{}_{}", name, id_tracker_.top());
        assoc_table_->insert(std::string(name), std::move(assoc));
        return assoc_table_->query(name);
    }
    const std::string &translate(const std::string &name) {
        if (translatable(name))
            return assoc_table_->query(name);
        else
            fatal_error("{} doesn't exists", name);
    }
    void enter_scope() {
        assoc_table_ = std::make_shared<SymbolAssocTable>(assoc_table_);
        id_tracker_.push(++current_scope_id_);
    }
    void leave_scope() {
        assoc_table_ = assoc_table_->outer();
        id_tracker_.pop();
        if (!assoc_table_) {
            fatal_error("leave from root scope");
        }
    }

private:
    class SymbolAssocTable {
    public:
        SymbolAssocTable() : outer_(nullptr) {}
        SymbolAssocTable(const std::shared_ptr<SymbolAssocTable> &outer)
            : outer_(outer) {}
        const std::shared_ptr<SymbolAssocTable> &outer() const {
            return outer_;
        }
        const std::string &query(const std::string &name) {
            if (map_.find(name) != map_.end()) {
                return map_.at(name);
            } else {
                if (!outer_) {
                    throw std::out_of_range(
                        fmt::format("{} doesn't exists", name));
                } else {
                    return outer_->query(name);
                }
            }
        }
        void insert(std::string &&symbol, std::string &&assoc) {
            map_.insert(std::make_pair(symbol, assoc));
        }
        bool exists(const std::string &name) {
            if (map_.find(name) != map_.end()) {
                return true;
            } else {
                return outer_ ? outer_->exists(name) : false;
            }
        }

    private:
        std::shared_ptr<SymbolAssocTable> outer_;
        std::map<std::string, std::string> map_;
    };

    std::shared_ptr<SymbolAssocTable> assoc_table_;
    std::stack<uint64_t> id_tracker_;
    uint64_t current_scope_id_;
};

class HirGenContext {
public:
    HirGenContext(Context &ctx) : ctx_(ctx), translator_() {}
    Context &ctx() { return ctx_; }
    VariableTranslator &translator() { return translator_; }

private:
    Context &ctx_;
    VariableTranslator translator_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_CONTEXT_H_
