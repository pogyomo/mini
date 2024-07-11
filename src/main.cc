#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include "context.h"
#include "hir/printable.h"
#include "hirgen/hirgen.h"
#include "panic.h"

class Arguments {
public:
    Arguments(int argc, char *argv[]) {
        std::optional<std::string> output;
        std::optional<std::string> input;
        bool emit_hir = false;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--emit-hir") {
                emit_hir = true;
            } else if (arg == "-o") {
                if (i < argc - 1) {
                    output.emplace(argv[++i]);
                } else {
                    mini::FatalError("expect output filename after -o");
                }
            } else {
                if (input) {
                    mini::FatalError("duplicated input file name");
                } else {
                    input.emplace(argv[i]);
                }
            }
        }
        if (!input) {
            mini::FatalError("expected input file name");
        } else {
            input_ = input.value();
            output_ = output;
            emit_hir_ = emit_hir;
        }
    }
    const std::string &input() const { return input_; }
    const std::optional<std::string> &output() const { return output_; }
    bool emit_hir() const { return emit_hir_; }

private:
    std::string input_;
    std::optional<std::string> output_;
    bool emit_hir_;
};

int main(int argc, char *argv[]) {
    Arguments args(argc, argv);
    if (args.emit_hir()) {
        mini::Context ctx;
        auto decls = mini::HirgenFile(ctx, args.input());
        if (!decls) std::exit(EXIT_FAILURE);
        if (args.output()) {
            std::ofstream ofs(args.output().value());
            if (ofs.bad()) {
                mini::FatalError("failed to open output file");
            }
            for (const auto &decl : decls.value()) {
                mini::hir::PrintableContext ctx(ofs, 4);
                decl->PrintLn(ctx);
                ctx.printer().PrintLn("");
            }
        } else {
            for (const auto &decl : decls.value()) {
                mini::hir::PrintableContext ctx(std::cout, 4);
                decl->PrintLn(ctx);
                ctx.printer().PrintLn("");
            }
        }
    } else {
        mini::FatalError("not yet implemented");
    }
}
