#ifndef MINI_CODEGEN_CONTEXT_H_
#define MINI_CODEGEN_CONTEXT_H_

#include <cstdint>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <utility>

#include "../context.h"
#include "../hir/root.h"
#include "../hir/type.h"

namespace mini {

class ArgReg {
public:
    enum Kind { R1, R2, R3, R4, R5, R6, Invalid };

    ArgReg() : kind_(R1) {}
    ArgReg(Kind kind) : kind_(kind) {}
    explicit operator bool() const { return kind_ != Invalid; }
    ArgReg &operator++() {
        if (kind_ != Invalid) {
            kind_ = static_cast<Kind>(kind_ + 1);
        }
        return *this;
    }
    ArgReg operator++(int) {
        ArgReg reg = *this;
        if (kind_ != Invalid) {
            kind_ = static_cast<Kind>(kind_ + 1);
        }
        return reg;
    }
    std::string ToRealReg() {
        switch (kind_) {
            case R1:
                return "%rdi";
            case R2:
                return "%rsi";
            case R3:
                return "%rdx";
            case R4:
                return "%rcx";
            case R5:
                return "%r8";
            case R6:
                return "%r9";
            case Invalid:
                FatalError("try to convert invalid register");
        }
    }
    Kind kind() const { return kind_; }

private:
    Kind kind_;
};

class LVarTable {
public:
    class Entry {
    public:
        Entry(uint64_t offset) : offset_(offset) {}

        // Retruns offset where `rbp - offset` points it.
        inline uint64_t offset() const { return offset_; }

    private:
        uint64_t offset_;
    };

    LVarTable() : size_(0) {}
    inline uint64_t size() const { return size_; }
    inline void AlignSize(uint64_t align) {
        while (size_ % align != 0) size_++;
    }
    inline void ChangeSize(uint64_t offset) { size_ = offset; }
    inline void AddSize(uint64_t diff) { size_ += diff; }
    inline void SubSize(uint64_t diff) { size_ -= diff; }
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
    uint64_t size_;
};

class StructTable {
public:
    class Entry {
    public:
        class Field {
        public:
            Field(const std::shared_ptr<hir::Type> &type) : type_(type) {}
            const std::shared_ptr<hir::Type> &type() const { return type_; }
            void set_offset(uint64_t offset) { offset_ = offset; }
            uint64_t offset() const { return offset_; }

        private:
            std::shared_ptr<hir::Type> type_;
            uint64_t offset_;
        };

        using iterator = std::map<std::string, Field>::iterator;
        using const_iterator = std::map<std::string, Field>::const_iterator;

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
        inline bool Exists(const std::string &name) {
            return fields_.find(name) != fields_.end();
        }
        inline void Insert(std::string &&name, Field &&type) {
            fields_.insert(std::make_pair(name, type));
        }
        Field &Query(const std::string &name) {
            if (!Exists(name)) {
                FatalError("no such struct field exists: {}", name);
            } else {
                return fields_.at(name);
            }
        }

    private:
        std::map<std::string, Field> fields_;
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
        bool Exists(const std::string &name) {
            return fields_.find(name) != fields_.end();
        }
        void Insert(std::string &&name, uint64_t value) {
            fields_.insert(std::make_pair(name, value));
        }
        uint64_t Query(const std::string &name) {
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

class FuncSigTable {
public:
    class Entry {
    public:
        Span span() const { return span_; }
        Entry(const std::shared_ptr<hir::Type> &ret_type, Span span)
            : ret_type_(ret_type), span_(span) {}
        inline const std::shared_ptr<hir::Type> &ret_type() const {
            return ret_type_;
        }
        inline bool ParamExists(const std::string &name) {
            return params_.find(name) != params_.end();
        }
        inline void InsertParam(std::string &&name,
                                const std::shared_ptr<hir::Type> &type) {
            params_.insert(std::make_pair(name, type));
        }
        const std::shared_ptr<hir::Type> &QueryParam(const std::string &name) {
            if (!ParamExists(name)) {
                FatalError("no such parameter exists: {}", name);
            } else {
                return params_.at(name);
            }
        }

    private:
        std::shared_ptr<hir::Type> ret_type_;
        std::map<std::string, std::shared_ptr<hir::Type>> params_;
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
    inline LVarTable &lvar_table() { return lvar_table_; }
    inline StructTable &struct_table() { return struct_table_; }
    inline EnumTable &enum_table() { return enum_table_; }
    inline FuncSigTable &func_sig_table() { return func_sig_table_; }
    inline LabelIdGenerator &label_id_generator() {
        return label_id_generator_;
    }

private:
    Context &ctx_;
    const hir::StringTable &string_table_;
    Printer printer_;
    LVarTable lvar_table_;
    StructTable struct_table_;
    EnumTable enum_table_;
    FuncSigTable func_sig_table_;
    LabelIdGenerator label_id_generator_;
};

}  // namespace mini

#endif  // MINI_CODEGEN_CONTEXT_H_
