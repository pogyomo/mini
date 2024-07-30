#ifndef MINI_REPORT_H_
#define MINI_REPORT_H_

#include <string>

#include "context.h"
#include "span.h"

namespace mini {

enum class ReportLevel {
    Info,
    Warn,
    Error,
};

class ReportInfo {
public:
    ReportInfo(Span span, std::string &&what, std::string &&info)
        : span_(span), what_(what), info_(info) {}
    inline Span span() const { return span_; }
    inline const std::string &what() const { return what_; }
    inline const std::string &info() const { return info_; }

private:
    const Span span_;
    const std::string what_;
    const std::string info_;
};

void Report(Context &ctx, ReportLevel level, const ReportInfo &info);

};  // namespace mini

#endif  // MINI_REPORT_H_
