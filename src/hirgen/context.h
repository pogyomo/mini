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

    // Returns true if the name is translatable.
    // Set `upward` false will checks translatability only at current scope.
    inline bool Translatable(const std::string &name, bool upward = true) {
        return assoc_table_->Exists(name, upward);
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

        // Get outer table.
        inline const std::shared_ptr<SymbolAssocTable> &outer() const {
            return outer_;
        }

        // Try to get name associated with `name`.
        const std::string &Query(const std::string &name);

        // Insert `symbol` with `assoc`.
        inline void Insert(std::string &&symbol, std::string &&assoc) {
            map_.insert(std::make_pair(symbol, assoc));
        }

        // Returns true if the `name` is registerd in thsi table.
        // Set `upward` false will ignore outer table.
        bool Exists(const std::string &name, bool upward);

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
