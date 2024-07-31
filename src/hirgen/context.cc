#include "context.h"

#include <stdexcept>

#include "fmt/format.h"

namespace mini {

const std::string &VariableTranslator::RegVar(const std::string &name) {
    auto assoc = fmt::format("{}_{}", name, id_tracker_.top());
    assoc_table_->Insert(std::string(name), std::move(assoc));
    return assoc_table_->Query(name);
}

const std::string &VariableTranslator::RegVarRaw(const std::string &name) {
    assoc_table_->Insert(std::string(name), std::string(name));
    return assoc_table_->Query(name);
}

const std::string &VariableTranslator::Translate(const std::string &name) {
    if (Translatable(name))
        return assoc_table_->Query(name);
    else
        FatalError("{} doesn't exists", name);
}

void VariableTranslator::EnterScope() {
    assoc_table_ = std::make_shared<SymbolAssocTable>(assoc_table_);
    id_tracker_.push(++current_scope_id_);
}

void VariableTranslator::LeaveScope() {
    assoc_table_ = assoc_table_->outer();
    id_tracker_.pop();
    if (!assoc_table_) {
        FatalError("leave from root scope");
    }
}

const std::string &VariableTranslator::SymbolAssocTable::Query(
    const std::string &name) {
    if (map_.find(name) != map_.end()) {
        return map_.at(name);
    } else {
        if (!outer_) {
            throw std::out_of_range(fmt::format("{} doesn't exists", name));
        } else {
            return outer_->Query(name);
        }
    }
}

bool VariableTranslator::SymbolAssocTable::Exists(const std::string &name) {
    if (map_.find(name) != map_.end()) {
        return true;
    } else {
        return outer_ ? outer_->Exists(name) : false;
    }
}

}  // namespace mini
