#ifndef MINI_CODEGEN_ASM_H_
#define MINI_CODEGEN_ASM_H_

#include <cstdint>
#include <string>

#include "../panic.h"

namespace mini {

// Abstract register in x86-64.
class Register {
public:
    enum Kind {
        AX,
        BX,
        CX,
        DX,
        SI,
        DI,
        BP,
        SP,
        R8,
        R9,
        R10,
        R11,
        R12,
        R13,
        R14,
        R15,
    };

    Register(Kind kind) : kind_(kind) {}

    // Returns the representation of this register in 1-byte.
    std::string ToByteName() const;

    // Returns the representation of this register in 2-byte.
    std::string ToWordName() const;

    // Returns the representation of this register in 4-byte.
    std::string ToLongName() const;

    // Returns the representation of this register in 8-byte.
    std::string ToQuadName() const;

    // Returns the representation of this register in `size`-byte.
    std::string ToNameBySize(uint8_t size) const;

private:
    Kind kind_;
};

// A pointer represented by a register.
class IndexableAsmRegPtr {
public:
    IndexableAsmRegPtr(Register reg, int64_t init_offset)
        : reg_(reg), init_offset_(init_offset) {}
    std::string ToAsmRepr(int64_t offset, uint8_t size) const;

private:
    Register reg_;
    int64_t init_offset_;
};

std::string AsmAdd(uint8_t size);
std::string AsmSub(uint8_t size);
std::string AsmMul(bool is_signed, uint8_t size);
std::string AsmDiv(bool is_signed, uint8_t size);
std::string AsmAnd(uint8_t size);
std::string AsmOr(uint8_t size);
std::string AsmXor(uint8_t size);
std::string AsmCmp(uint8_t size);
std::string AsmNot(uint8_t size);
std::string AsmNeg(uint8_t size);
std::string AsmLShift(bool is_signed, uint8_t size);
std::string AsmRShift(bool is_signed, uint8_t size);

class CodeGenContext;

// Generate code which copy `size` bytes from `src` to `dst`.
void CopyBytes(CodeGenContext& ctx, const IndexableAsmRegPtr& src,
               const IndexableAsmRegPtr& dst, uint64_t size);

}  // namespace mini

#endif  // MINI_CODEGEN_ASM_H_
