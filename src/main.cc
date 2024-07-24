#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#include "codegen/codegen.h"
#include "context.h"
#include "hirgen/hirgen.h"
#include "panic.h"

class Arguments {
public:
    Arguments(int argc, char *argv[]) {
        std::optional<std::string> output;
        std::optional<std::string> input;
        bool emit_hir = false;
        bool emit_asm = false;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--emit-hir") {
                if (emit_asm) {
                    mini::FatalError("cannot use --emit-hir with -S");
                }
                emit_hir = true;
            } else if (arg == "-S") {
                if (emit_asm) {
                    mini::FatalError("cannot use -S with --emit-hir");
                }
                emit_asm = true;
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
            emit_asm_ = emit_asm;
        }
    }
    const std::string &input() const { return input_; }
    const std::optional<std::string> &output() const { return output_; }
    bool emit_hir() const { return emit_hir_; }
    bool emit_asm() const { return emit_asm_; }

private:
    std::string input_;
    std::optional<std::string> output_;
    bool emit_hir_;
    bool emit_asm_;
};

int main(int argc, char *argv[]) {
    Arguments args(argc, argv);
    if (args.emit_hir()) {
        mini::Context ctx;
        auto root = mini::HirGenFile(ctx, args.input());
        if (!root) std::exit(EXIT_FAILURE);
        if (args.output()) {
            std::ofstream ofs(args.output().value());
            if (ofs.bad()) {
                mini::FatalError("failed to open output file");
            }
            mini::hir::PrintableContext ctx(ofs, 4);
            root->PrintLn(ctx);
        } else {
            mini::hir::PrintableContext ctx(std::cout, 4);
            root->PrintLn(ctx);
        }
    } else if (args.emit_asm()) {
        if (args.output()) {
            std::ofstream ofs(args.output().value());
            if (ofs.bad()) {
                mini::FatalError("failed to open output file");
            }

            mini::Context ctx;
            auto root = mini::CodeGenFile(ctx, ofs, args.input());
            if (!root) std::exit(EXIT_FAILURE);
        } else {
            mini::Context ctx;
            auto root = mini::CodeGenFile(ctx, std::cout, args.input());
            if (!root) std::exit(EXIT_FAILURE);
        }
    }
}
