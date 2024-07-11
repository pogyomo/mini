#ifndef MINI_HIR_PRINTABLE_H_
#define MINI_HIR_PRINTABLE_H_

#include <cstdint>
#include <ostream>
#include <string>

#include "../panic.h"

namespace mini {

namespace hir {

class AutoIndentPrinter {
public:
    AutoIndentPrinter(std::ostream &os, uint16_t width)
        : os_(os), depth_(0), width_(width) {}
    inline void ShiftR() { depth_++; }
    inline void ShiftL() {
        if (depth_)
            depth_--;
        else
            FatalError("shiftl with depth == 0");
    }
    inline uint16_t depth() const { return depth_; }
    inline void Print(const std::string &s) { os_ << s; }
    inline void PrintLn(const std::string &s) {
        Print(s);
        os_ << std::endl << std::string(depth_ * width_, ' ');
    }

private:
    std::ostream &os_;
    uint16_t depth_;
    const uint16_t width_;
};

class PrintableContext {
public:
    PrintableContext(std::ostream &os, uint16_t width) : printer_(os, width) {}
    AutoIndentPrinter &printer() { return printer_; }

private:
    AutoIndentPrinter printer_;
};

class Printable {
public:
    virtual ~Printable() {}
    virtual void Print(PrintableContext &ctx) const = 0;
    virtual void PrintLn(PrintableContext &ctx) const {
        Print(ctx);
        ctx.printer().PrintLn("");
    }
};

}  // namespace hir

}  // namespace mini

#endif  // MINI_HIR_PRINTABLE_H_
