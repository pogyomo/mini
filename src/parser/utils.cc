#include "utils.h"

#include "../report.h"

namespace mini {

bool check_eos(Context &ctx, TokenStream &ts) {
    if (!ts) {
        ReportInfo info(ts.Last()->span(), "expected token after this", "");
        Report(ctx, ReportLevel::Error, info);
        return true;
    } else {
        return false;
    }
}

bool check_ident(Context &ctx, TokenStream &ts) {
    if (check_eos(ctx, ts)) {
        return true;
    } else {
        if (!ts.CurrToken()->IsIdent()) {
            if (ts.HasPrev()) {
                ReportInfo info(ts.PrevToken()->span(),
                                "expected identifier after this", "");
                Report(ctx, ReportLevel::Error, info);
                return true;
            } else {
                ReportInfo info(ts.CurrToken()->span(),
                                "expected this to be identifier", "");
                Report(ctx, ReportLevel::Error, info);
                return true;
            }
        } else {
            return false;
        }
    }
}

bool check_punct(Context &ctx, TokenStream &ts, PunctTokenKind kind) {
    if (check_eos(ctx, ts)) {
        return true;
    } else {
        if (!ts.CurrToken()->IsPunctOf(kind)) {
            if (ts.HasPrev()) {
                ReportInfo info(ts.PrevToken()->span(),
                                "expected `" + ToString(kind) + "` after this",
                                "");
                Report(ctx, ReportLevel::Error, info);
                return true;
            } else {
                ReportInfo info(ts.CurrToken()->span(),
                                "expected this to be `" + ToString(kind) + "`",
                                "");
                Report(ctx, ReportLevel::Error, info);
                return true;
            }
        } else {
            return false;
        }
    }
}

bool check_keyword(Context &ctx, TokenStream &ts, KeywordTokenKind kind) {
    if (check_eos(ctx, ts)) {
        return true;
    } else {
        if (!ts.CurrToken()->IsKeywordOf(kind)) {
            if (ts.HasPrev()) {
                ReportInfo info(ts.PrevToken()->span(),
                                "expected `" + ToString(kind) + "` after this",
                                "");
                Report(ctx, ReportLevel::Error, info);
                return true;
            } else {
                ReportInfo info(ts.CurrToken()->span(),
                                "expected this to be `" + ToString(kind) + "`",
                                "");
                Report(ctx, ReportLevel::Error, info);
                return true;
            }
        } else {
            return false;
        }
    }
}

}  // namespace mini
