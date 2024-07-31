#ifndef MINI_HIRGEN_CONTEXT_H_
#define MINI_HIRGEN_CONTEXT_H_

#include <map>
#include <memory>
#include <stack>
#include <string>

#include "../context.h"
#include "../hir/root.h"

namespace mini {

class VariableTranslator {
public:
    VariableTranslator()
        : assoc_table_(std::make_shared<SymbolAssocTable>()),
          id_tracker_({0}),
          current_scope_id_(0) {}
    inline bool Translatable(const std::string &name) {
        return assoc_table_->Exists(name);
    }
    const std::string &RegVar(const std::string &name);
    const std::string &RegVarRaw(const std::string &name);
    const std::string &Translate(const std::string &name);
    void EnterScope();
    void LeaveScope();

private:
    class SymbolAssocTable {
    public:
        SymbolAssocTable() : outer_(nullptr) {}
        SymbolAssocTable(const std::shared_ptr<SymbolAssocTable> &outer)
            : outer_(outer) {}
        inline const std::shared_ptr<SymbolAssocTable> &outer() const {
            return outer_;
        }
        const std::string &Query(const std::string &name);
        inline void Insert(std::string &&symbol, std::string &&assoc) {
            map_.insert(std::make_pair(symbol, assoc));
        }
        bool Exists(const std::string &name);

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
    HirGenContext(Context &ctx, hir::StringTable &string_table)
        : ctx_(ctx), string_table_(string_table), translator_() {}
    Context &ctx() { return ctx_; }
    hir::StringTable &string_table() { return string_table_; }
    VariableTranslator &translator() { return translator_; }

private:
    Context &ctx_;
    hir::StringTable &string_table_;
    VariableTranslator translator_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_CONTEXT_H_
