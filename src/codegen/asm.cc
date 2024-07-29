#include "asm.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "context.h"
#include "fmt/format.h"

namespace mini {

std::string Register::ToByteName() const {
    switch (kind_) {
        case AX:
            return "%al";
        case BX:
            return "%bl";
        case CX:
            return "%cl";
        case DX:
            return "%dl";
        case SI:
            return "%sil";
        case DI:
            return "%dil";
        case BP:
            return "%bpl";
        case SP:
            return "%spl";
        case R8:
            return "%r8b";
        case R9:
            return "%r9b";
        case R10:
            return "%r10b";
        case R11:
            return "%r11b";
        case R12:
            return "%r12b";
        case R13:
            return "%r13b";
        case R14:
            return "%r14b";
        case R15:
            return "%r15b";
        default:
            FatalError("unreachable");
    }
}

std::string Register::ToWordName() const {
    switch (kind_) {
        case AX:
            return "%ax";
        case BX:
            return "%bx";
        case CX:
            return "%cx";
        case DX:
            return "%dx";
        case SI:
            return "%si";
        case DI:
            return "%di";
        case BP:
            return "%bp";
        case SP:
            return "%sp";
        case R8:
            return "%r8w";
        case R9:
            return "%r9w";
        case R10:
            return "%r10w";
        case R11:
            return "%r11w";
        case R12:
            return "%r12w";
        case R13:
            return "%r13w";
        case R14:
            return "%r14w";
        case R15:
            return "%r15w";
        default:
            FatalError("unreachable");
    }
}

std::string Register::ToLongName() const {
    switch (kind_) {
        case AX:
            return "%eax";
        case BX:
            return "%ebx";
        case CX:
            return "%ecx";
        case DX:
            return "%edx";
        case SI:
            return "%esi";
        case DI:
            return "%edi";
        case BP:
            return "%ebp";
        case SP:
            return "%esp";
        case R8:
            return "%r8d";
        case R9:
            return "%r9d";
        case R10:
            return "%r10d";
        case R11:
            return "%r11d";
        case R12:
            return "%r12d";
        case R13:
            return "%r13d";
        case R14:
            return "%r14d";
        case R15:
            return "%r15d";
        default:
            FatalError("unreachable");
    }
}

std::string Register::ToQuadName() const {
    switch (kind_) {
        case AX:
            return "%rax";
        case BX:
            return "%rbx";
        case CX:
            return "%rcx";
        case DX:
            return "%rdx";
        case SI:
            return "%rsi";
        case DI:
            return "%rdi";
        case BP:
            return "%rbp";
        case SP:
            return "%rsp";
        case R8:
            return "%r8";
        case R9:
            return "%r9";
        case R10:
            return "%r10";
        case R11:
            return "%r11";
        case R12:
            return "%r12";
        case R13:
            return "%r13";
        case R14:
            return "%r14";
        case R15:
            return "%r15";
        default:
            FatalError("unreachable");
    }
}

std::string Register::ToNameBySize(uint8_t size) const {
    if (size == 1) {
        return ToByteName();
    } else if (size == 2) {
        return ToWordName();
    } else if (size == 4) {
        return ToLongName();
    } else if (size == 8) {
        return ToQuadName();
    } else {
        FatalError("invalid size: {}", size);
    }
}

std::string IndexableAsmRegPtr::ToAsmRepr(int64_t offset, uint8_t size) const {
    return fmt::format("{}({})", init_offset_ + offset,
                       reg_.ToNameBySize(size));
}

std::string AsmAdd(uint8_t size) {
    if (size == 1) {
        return "addb";
    } else if (size == 2) {
        return "addw";
    } else if (size == 4) {
        return "addl";
    } else if (size == 8) {
        return "addq";
    } else {
        FatalError("invalid size");
    }
}

std::string AsmSub(uint8_t size) {
    if (size == 1) {
        return "subb";
    } else if (size == 2) {
        return "subw";
    } else if (size == 4) {
        return "subl";
    } else if (size == 8) {
        return "subq";
    } else {
        FatalError("invalid size");
    }
}

std::string AsmMul(bool is_signed, uint8_t size) {
    if (is_signed) {
        if (size == 1) {
            return "imulb";
        } else if (size == 2) {
            return "imulw";
        } else if (size == 4) {
            return "imull";
        } else if (size == 8) {
            return "imulq";
        } else {
            FatalError("invalid size");
        }
    } else {
        if (size == 1) {
            return "mulb";
        } else if (size == 2) {
            return "mulw";
        } else if (size == 4) {
            return "mull";
        } else if (size == 8) {
            return "mulq";
        } else {
            FatalError("invalid size");
        }
    }
}

std::string AsmDiv(bool is_signed, uint8_t size) {
    if (is_signed) {
        if (size == 1) {
            return "idivb";
        } else if (size == 2) {
            return "idivw";
        } else if (size == 4) {
            return "idivl";
        } else if (size == 8) {
            return "idivq";
        } else {
            FatalError("invalid size");
        }
    } else {
        if (size == 1) {
            return "divb";
        } else if (size == 2) {
            return "divw";
        } else if (size == 4) {
            return "divl";
        } else if (size == 8) {
            return "divq";
        } else {
            FatalError("invalid size");
        }
    }
}

std::string AsmAnd(uint8_t size) {
    if (size == 1) {
        return "andb";
    } else if (size == 2) {
        return "andw";
    } else if (size == 4) {
        return "andl";
    } else if (size == 8) {
        return "andq";
    } else {
        FatalError("invalid size");
    }
}

std::string AsmOr(uint8_t size) {
    if (size == 1) {
        return "orb";
    } else if (size == 2) {
        return "orw";
    } else if (size == 4) {
        return "orl";
    } else if (size == 8) {
        return "orq";
    } else {
        FatalError("invalid size");
    }
}

std::string AsmXor(uint8_t size) {
    if (size == 1) {
        return "xorb";
    } else if (size == 2) {
        return "xorw";
    } else if (size == 4) {
        return "xorl";
    } else if (size == 8) {
        return "xorq";
    } else {
        FatalError("invalid size");
    }
}

std::string AsmCmp(uint8_t size) {
    if (size == 1) {
        return "cmpb";
    } else if (size == 2) {
        return "cmpw";
    } else if (size == 4) {
        return "cmpl";
    } else if (size == 8) {
        return "cmpq";
    } else {
        FatalError("invalid size");
    }
}

std::string AsmNot(uint8_t size) {
    if (size == 1) {
        return "notb";
    } else if (size == 2) {
        return "notw";
    } else if (size == 4) {
        return "notl";
    } else if (size == 8) {
        return "notq";
    } else {
        FatalError("invalid size");
    }
}

std::string AsmNeg(uint8_t size) {
    if (size == 1) {
        return "negb";
    } else if (size == 2) {
        return "negw";
    } else if (size == 4) {
        return "negl";
    } else if (size == 8) {
        return "negq";
    } else {
        FatalError("invalid size");
    }
}

std::string AsmLShift(bool is_signed, uint8_t size) {
    if (is_signed) {
        if (size == 1) {
            return "salb";
        } else if (size == 2) {
            return "salw";
        } else if (size == 4) {
            return "sall";
        } else if (size == 8) {
            return "salq";
        } else {
            FatalError("invalid size");
        }
    } else {
        if (size == 1) {
            return "shlb";
        } else if (size == 2) {
            return "shlw";
        } else if (size == 4) {
            return "shll";
        } else if (size == 8) {
            return "shlq";
        } else {
            FatalError("invalid size");
        }
    }
}

std::string AsmRShift(bool is_signed, uint8_t size) {
    if (is_signed) {
        if (size == 1) {
            return "sarb";
        } else if (size == 2) {
            return "sarw";
        } else if (size == 4) {
            return "sarl";
        } else if (size == 8) {
            return "sarq";
        } else {
            FatalError("invalid size");
        }
    } else {
        if (size == 1) {
            return "shrb";
        } else if (size == 2) {
            return "shrw";
        } else if (size == 4) {
            return "shrl";
        } else if (size == 8) {
            return "shrq";
        } else {
            FatalError("invalid size");
        }
    }
}

void CopyBytes(CodeGenContext& ctx, const IndexableAsmRegPtr& src,
               const IndexableAsmRegPtr& dst, uint64_t size) {
    static uint8_t sizes[4] = {8, 4, 2, 1};
    static std::string moves[4] = {"movq", "movl", "movw", "movb"};
    int64_t offset = 0;
    while (size != 0) {
        Register tmp_reg(Register::CX);
        for (size_t i = 0; i < sizeof sizes / sizeof sizes[0]; i++) {
            if (size >= sizes[i]) {
                ctx.printer().PrintLn("    {} {}, {}", moves[i],
                                      src.ToAsmRepr(offset, 8),
                                      tmp_reg.ToNameBySize(sizes[i]));
                ctx.printer().PrintLn("    {} {}, {}", moves[i],
                                      tmp_reg.ToNameBySize(sizes[i]),
                                      dst.ToAsmRepr(offset, 8));
                offset += sizes[i];
                size -= sizes[i];
                break;
            }
        }
    }
}

}  // namespace mini
