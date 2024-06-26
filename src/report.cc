#include "report.h"

#include <iostream>
#include <ostream>

// Start to coloring the output to `os` depende on `level`.
static void start_color(std::ostream &os, ReportLevel level) {
    if (level == ReportLevel::Error) {
        os << "\e[31m";
    } else if (level == ReportLevel::Warn) {
        os << "\e[33m";
    } else {
        os << "\e[34m";
    }
}

// Stop to coloring the output to `os`.
static void end_color(std::ostream &os) { os << "\e[0m"; }

// Returns the number of digits in `n`.
static int digits(int n) {
    if (n == 0) return 1;
    int res = 0;
    while (n > 0) {
        res++;
        n /= 10;
    }
    return res;
}

void report(Context &ctx, ReportLevel level, const ReportInfo &info) {
    if (ctx.suppress_report()) {
        return;
    }

    auto entry = ctx.input_cache().fetch(info.span().id());
    auto start = info.span().start();
    auto end = info.span().end();

    std::cerr << entry.name() << ":" << start.row() << ":" << start.offset()
              << ":";
    start_color(std::cerr, level);
    if (level == ReportLevel::Error) {
        std::cerr << "error: ";
    } else if (level == ReportLevel::Warn) {
        std::cerr << "warning: ";
    } else {
        std::cerr << "info: ";
    }
    end_color(std::cerr);
    std::cerr << info.what() << std::endl;

    int row_width = digits(start.row()) > digits(end.row())
                        ? digits(start.row())
                        : digits(end.row());
    if (start.row() == end.row()) {
        auto line = entry.lines().at(start.row());
        std::cerr << "  " << start.row() << "|" << line << std::endl;
        for (int i = 0; i < 2 + row_width; i++) std::cerr << ' ';
        std::cerr << "|";
        for (int i = 0; i < start.offset(); i++) std::cerr << ' ';
        start_color(std::cerr, level);
        std::cerr << '^';
        for (int i = start.offset() + 1; i <= end.offset(); i++)
            std::cerr << '~';
        end_color(std::cerr);
        std::cerr << " " << info.info() << std::endl;
    } else {
        auto sline = entry.lines().at(start.row());
        std::cerr << "  ";
        for (int i = 0; i < row_width - digits(start.row()); i++)
            std::cerr << '0';
        std::cerr << start.row() << "|" << sline << std::endl;
        for (int i = 0; i < 2 + row_width; i++) std::cerr << ' ';
        std::cerr << '|';
        for (int i = 0; i < start.offset(); i++) std::cerr << ' ';
        start_color(std::cerr, level);
        std::cerr << '^';
        for (int i = start.offset() + 1; i < sline.size(); i++)
            std::cerr << '~';
        end_color(std::cerr);
        std::cerr << std::endl;

        std::cerr << "  ";
        for (int i = 0; i < row_width; i++) std::cerr << ' ';
        std::cerr << ":" << std::endl;

        auto eline = entry.lines().at(end.row());
        std::cerr << "  ";
        for (int i = 0; i < row_width - digits(end.row()); i++)
            std::cerr << '0';
        std::cerr << end.row() << "|" << eline << std::endl;
        for (int i = 0; i < 2 + row_width; i++) std::cerr << ' ';
        std::cerr << '|';
        start_color(std::cerr, level);
        for (int i = 0; i <= end.offset(); i++) std::cerr << '~';
        end_color(std::cerr);
        std::cerr << " " << info.info() << std::endl;
    }
}
