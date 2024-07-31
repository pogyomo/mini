#ifndef MINI_HIRGEN_CONTEXT_H_
#define MINI_HIRGEN_CONTEXT_H_

#include <map>
#include <memory>
#include <string>

#include "../context.h"
#include "../hir/root.h"

namespace mini {

class NameTranslator {
public:
    NameTranslator()
        : assoc_table_(std::make_shared<SymbolAssocTable>()), curr_id_(0) {}

    // Returns true if the name is registered at current or parent scopes.
    inline bool Translatable(const std::string &name) {
        return assoc_table_->Exists(name);
    }

    // Register name and associate it with unique name.
    const std::string &RegName(const std::string &name);

    // Register name but associate with itself.
    const std::string &RegNameRaw(const std::string &name);

    // Translate given name into associated name.
    const std::string &Translate(const std::string &name);

    void EnterFunc() { curr_id_ = 0; }
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
    uint64_t curr_id_;
};

class HirGenContext {
public:
    HirGenContext(Context &ctx, hir::StringTable &string_table)
        : ctx_(ctx), string_table_(string_table), translator_() {}
    Context &ctx() { return ctx_; }
    hir::StringTable &string_table() { return string_table_; }
    NameTranslator &translator() { return translator_; }

private:
    Context &ctx_;
    hir::StringTable &string_table_;
    NameTranslator translator_;
};

}  // namespace mini

#endif  // MINI_HIRGEN_CONTEXT_H_
