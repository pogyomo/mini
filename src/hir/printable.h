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
        : os_(os), width_(width), depth_(0) {}
    void shiftr() { depth_++; }
    void shiftl() {
        if (depth_)
            depth_--;
        else
            fatal_error("shiftl with depth == 0");
    }
    uint16_t depth() const { return depth_; }
    void print(const std::string &s) { os_ << s; }
    void println(const std::string &s) {
        print(s);
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
    virtual void print(PrintableContext &ctx) const = 0;
    virtual void println(PrintableContext &ctx) const {
        print(ctx);
        ctx.printer().println("");
    }
};

}  // namespace hir

}  // namespace mini

#endif  // MINI_HIR_PRINTABLE_H_
