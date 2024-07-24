#ifndef MINI_CODEGEN_CONTEXT_H_
#define MINI_CODEGEN_CONTEXT_H_

#include <cassert>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include "../context.h"
#include "../hir/root.h"
#include "../hir/type.h"
#include "fmt/format.h"

namespace mini {

class LVarTable {
public:
    class Entry {
    public:
        Entry(bool is_caller_alloc, bool should_init_with_reg, uint8_t init_reg,
              uint64_t offset, const std::shared_ptr<hir::Type> &type)
            : is_caller_alloc_(is_caller_alloc),
              should_init_with_reg_(should_init_with_reg),
              init_reg_(init_reg),
              offset_(offset),
              type_(type) {}

        const std::shared_ptr<hir::Type> &type() const { return type_; }

        // Returns true if the variable should be initialized with register:
        // the variable is arguments.
        inline bool ShouldInitializeWithReg() const {
            return should_init_with_reg_;
        }

        // Returns the position register which the variable should be
        // initialized using it. Valid in the case of ShouldInitializeWithReg
        // returns true.
        inline uint8_t InitReg() const {
            assert(init_reg_ < 6);
            return init_reg_;
        }

        // Returns the name of register corresponding to the value of `InitReg`.
        // This contains `%` at beginning so that ready to use in assembly code.
        inline std::string InitRegName() const {
            static std::string aregs[] = {"%rdi", "%rsi", "%rdx",
                                          "%rcx", "%r8",  "%r9"};
            auto pos = InitReg();
            return aregs[pos];
        }

        // Returns true if the variable is allocated by caller:
        // the variable can be accessed with rbp + 16 + offset.
        // Otherwise the variable is allocated by callee and
        // accessable with rbp - offset.
        inline bool IsCallerAlloc() const { return is_caller_alloc_; }

        // Retruns offset where the value exists at
        // - `rbp-offset` is_caller_alloc == false
        // - `rbp+16+offset` is_caller_alloc == true
        inline uint64_t offset() const { return offset_; }

        // Returns the representaion of this variable in assembly code.
        //
        // example:
        // - when IsCallerAlloc == true and offset == 8, returns "24(%rbp)"
        // - when IsCallerAlloc == false and offset == 8, returns "-8(%rbp)"
        inline std::string AsmRepr() const {
            if (is_caller_alloc_) {
                return fmt::format("{}(%rbp)", offset_ + 16);
            } else {
                return fmt::format("-{}(%rbp)", offset_);
            }
        }

    private:
        bool is_caller_alloc_;
        bool should_init_with_reg_;
        uint8_t init_reg_;
        uint64_t offset_;
        std::shared_ptr<hir::Type> type_;
    };

    LVarTable() : callee_size_(0), caller_size_(0) {}

    inline uint64_t CalleeSize() const { return callee_size_; }
    inline void AlignCalleeSize(uint64_t align) {
        while (callee_size_ % align != 0) callee_size_++;
    }
    inline void ChangeCalleeSize(uint64_t offset) { callee_size_ = offset; }
    inline void AddCalleeSize(uint64_t diff) { callee_size_ += diff; }
    inline void SubCalleeSize(uint64_t diff) { callee_size_ -= diff; }

    inline uint64_t CallerSize() const { return caller_size_; }
    inline void AlignCallerSize(uint64_t align) {
        while (caller_size_ % align != 0) caller_size_++;
    }
    inline void ChangeCallerSize(uint64_t offset) { caller_size_ = offset; }
    inline void AddCallerSize(uint64_t diff) { caller_size_ += diff; }
    inline void SubCallerSize(uint64_t diff) { caller_size_ -= diff; }

    inline void Clear() { map_.clear(); }
    inline bool Exists(const std::string &name) const {
        return map_.find(name) != map_.end();
    }
    inline void Insert(std::string &&name, Entry &&entry) {
        map_.insert(std::make_pair(name, entry));
    }
    const Entry &Query(const std::string &name) const {
        if (!Exists(name)) {
            FatalError("");
        } else {
            return map_.at(name);
        }
    };

private:
    std::map<std::string, Entry> map_;
    uint64_t callee_size_;
    uint64_t caller_size_;
};

class StructTable {
public:
    class Entry {
    public:
        class Field {
        public:
            Field(const std::shared_ptr<hir::Type> &type)
                : type_(type), offset_(0) {}
            const std::shared_ptr<hir::Type> &type() const { return type_; }
            void set_offset(uint64_t offset) { offset_ = offset; }
            uint64_t offset() const { return offset_; }

        private:
            std::shared_ptr<hir::Type> type_;
            uint64_t offset_;
        };

        using map = std::vector<std::pair<std::string, Field>>;
        using iterator = map::iterator;
        using const_iterator = map::const_iterator;
        using reference = map::reference;
        using const_reference = map::const_reference;
        using size_type = map::size_type;

        Entry(Span span)
            : size_and_offset_calculated_(false),
              align_calculated_(false),
              size_(0),
              align_(0),
              span_(span) {}
        iterator begin() { return fields_.begin(); }
        const_iterator begin() const { return fields_.begin(); }
        iterator end() { return fields_.end(); }
        const_iterator end() const { return fields_.end(); }
        reference at(size_type n) { return fields_.at(n); }
        const_reference at(size_type n) const { return fields_.at(n); }

        inline bool SizeAndOffsetCalculated() const {
            return size_and_offset_calculated_;
        }
        inline bool AlignCalculated() const { return align_calculated_; }
        inline void MarkAsSizeAndOffsetCalculated() {
            size_and_offset_calculated_ = true;
        }
        inline void MarkAsAlignCalculated() { align_calculated_ = true; }
        inline void set_size(uint64_t size) { size_ = size; }
        inline void set_align(uint64_t align) { align_ = align; }
        inline uint64_t size() const { return size_; }
        inline uint64_t align() const { return align_; }

        inline Span span() const { return span_; }
        inline bool Exists(const std::string &name) const {
            for (const auto &[name_, _] : fields_) {
                if (name == name_) return true;
            }
            return false;
        }
        inline void Insert(std::string &&name, Field &&type) {
            if (!Exists(name)) {
                fields_.emplace_back(std::make_pair(name, type));
            } else {
                FatalError("{} already exists as struct field", name);
            }
        }
        Field &Query(const std::string &name) {
            for (auto &[name_, field] : fields_) {
                if (name == name_) return field;
            }
            FatalError("no such struct field exists: {}", name);
        }

    private:
        map fields_;
        bool size_and_offset_calculated_;
        bool align_calculated_;
        uint64_t size_;
        uint64_t align_;
        Span span_;
    };

    inline bool Exists(const std::string &name) {
        return map_.find(name) != map_.end();
    }
    inline void Insert(std::string &&name, Entry &&entry) {
        map_.insert(std::make_pair(name, entry));
    }
    Entry &Query(const std::string &name) {
        if (!Exists(name)) {
            FatalError("no such struct exists: {}", name);
        } else {
            return map_.at(name);
        }
    }

private:
    std::map<std::string, Entry> map_;
};

class EnumTable {
public:
    class Entry {
    public:
        Entry(Span span) : span_(span) {}
        Span span() const { return span_; }
        bool Exists(const std::string &name) const {
            return fields_.find(name) != fields_.end();
        }
        void Insert(std::string &&name, uint64_t value) {
            fields_.insert(std::make_pair(name, value));
        }
        uint64_t Query(const std::string &name) const {
            if (!Exists(name)) {
                FatalError("no such enum field exists: {}", name);
            } else {
                return fields_.at(name);
            }
        }

    private:
        std::map<std::string, uint64_t> fields_;
        Span span_;
    };

    inline bool Exists(const std::string &name) {
        return map_.find(name) != map_.end();
    }
    inline void Insert(std::string &&name, Entry &&entry) {
        map_.insert(std::make_pair(name, entry));
    }
    const Entry &Query(const std::string &name) {
        if (!Exists(name)) {
            FatalError("no such enum exists: {}", name);
        } else {
            return map_.at(name);
        }
    }

private:
    std::map<std::string, Entry> map_;
};

class FuncInfoTable {
public:
    class Entry {
    public:
        class Params {
        public:
            // NOTE:
            // I use vector instead of map because
            // - Its holds original order of parameters.
            // - Most cases parameters < 10, so no effect to speed.
            using map =
                std::vector<std::pair<std::string, std::shared_ptr<hir::Type>>>;
            using iterator = map::iterator;
            using const_iterator = map::const_iterator;
            using size_type = map::size_type;
            using reference = map::reference;
            using const_reference = map::const_reference;

            iterator begin() { return map_.begin(); }
            const_iterator begin() const { return map_.begin(); }
            iterator end() { return map_.end(); }
            const_iterator end() const { return map_.end(); }
            inline size_type size() const { return map_.size(); }
            inline reference at(size_type n) { return map_.at(n); }
            inline const_reference at(size_type n) const { return map_.at(n); }

            bool Exists(const std::string &name) const {
                for (const auto &[name_, _] : map_) {
                    if (name == name_) return true;
                }
                return false;
            }
            void Insert(std::string &&name,
                        const std::shared_ptr<hir::Type> &type) {
                if (!Exists(name)) {
                    map_.emplace_back(std::make_pair(name, type));
                } else {
                    FatalError("{} already exists as parameter", name);
                }
            }
            const std::shared_ptr<hir::Type> &Query(const std::string &name) {
                for (const auto &[name_, type] : map_) {
                    if (name == name_) return type;
                }
                FatalError("no such parameter exists: {}", name);
            }

        private:
            map map_;
        };

        Span span() const { return span_; }
        Entry(const std::shared_ptr<hir::Type> &ret_type, Span span)
            : ret_type_(ret_type), span_(span) {}
        inline const std::shared_ptr<hir::Type> &ret_type() const {
            return ret_type_;
        }
        inline Params &params() { return params_; }
        inline LVarTable &lvar_table() { return lvar_table_; }

    private:
        std::shared_ptr<hir::Type> ret_type_;
        Params params_;
        LVarTable lvar_table_;
        Span span_;
    };

    inline bool Exists(const std::string &name) {
        return map_.find(name) != map_.end();
    }
    inline void Insert(std::string &&name, Entry &&entry) {
        map_.insert(std::make_pair(name, entry));
    }
    Entry &Query(const std::string &name) {
        if (!Exists(name)) {
            FatalError("no such function exists: {}", name);
        } else {
            return map_.at(name);
        }
    }

private:
    std::map<std::string, Entry> map_;
};

class Printer {
public:
    Printer(std::ostream &os) : os_(os) {}

    template <typename... T>
    inline void Print(fmt::format_string<T...> fmt, T &&...args) {
        os_ << fmt::format(fmt, std::forward<T>(args)...);
    }

    template <typename... T>
    inline void PrintLn(fmt::format_string<T...> fmt, T &&...args) {
        os_ << fmt::format(fmt, std::forward<T>(args)...) << std::endl;
    }

private:
    std::ostream &os_;
};

class LabelIdGenerator {
public:
    LabelIdGenerator() : curr_id_(0), next_id_(0) {}
    uint64_t GenNewId() { return curr_id_ = next_id_++; }
    uint64_t CurrId() { return curr_id_; }

private:
    uint64_t curr_id_;
    uint64_t next_id_;
};

class CodeGenContext {
public:
    CodeGenContext(Context &ctx, const hir::StringTable &string_table,
                   std::ostream &os)
        : ctx_(ctx), string_table_(string_table), printer_(os) {}
    inline Context &ctx() { return ctx_; }
    inline const hir::StringTable &string_table() { return string_table_; }
    inline Printer &printer() { return printer_; }
    inline StructTable &struct_table() { return struct_table_; }
    inline EnumTable &enum_table() { return enum_table_; }
    inline FuncInfoTable &func_info_table() { return func_info_table_; }
    inline LVarTable &lvar_table() {
        return func_info_table_.Query(curr_func_name_).lvar_table();
    }
    inline LabelIdGenerator &label_id_generator() {
        return label_id_generator_;
    }
    inline void SetCurrFuncName(std::string &&name) { curr_func_name_ = name; }
    inline const std::string &CurrFuncName() const { return curr_func_name_; }

private:
    Context &ctx_;
    const hir::StringTable &string_table_;
    Printer printer_;
    StructTable struct_table_;
    EnumTable enum_table_;
    FuncInfoTable func_info_table_;
    LabelIdGenerator label_id_generator_;
    std::string curr_func_name_;
};

}  // namespace mini

#endif  // MINI_CODEGEN_CONTEXT_H_
