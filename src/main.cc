#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <optional>
#include <ostream>
#include <string>

#include "codegen/codegen.h"
#include "context.h"
#include "fmt/format.h"
#include "hirgen/hirgen.h"
#include "panic.h"

enum class UsageKind {
    Normal,
    DuplicatedInput,
    UnknownOption,
    NoInputFile,
};

[[noreturn]] static void usage(std::ostream &os, UsageKind kind) {
    os << "Usage: mini <FILENAME> [ -o <OUTPUT> ]" << std::endl;
    os << "  -o filename Output to specified file" << std::endl;
    os << "  -S          Output assembly code" << std::endl;
    os << "  --emit-hir  Output internal representation" << std::endl;
    os << "  -h          Print this help" << std::endl;
    if (kind == UsageKind::DuplicatedInput) {
        mini::FatalError("duplicated input");
    } else if (kind == UsageKind::UnknownOption) {
        mini::FatalError("unknown option passed");
    } else if (kind == UsageKind::NoInputFile) {
        mini::FatalError("no input file");
    } else {
        std::exit(EXIT_SUCCESS);
    }
}

static bool startwith(const std::string &pattern, const std::string &target) {
    if (target.size() < pattern.size()) {
        return false;
    }
    for (size_t i = 0; i < pattern.size(); i++) {
        if (target.at(i) != pattern.at(i)) return false;
    }
    return true;
}

class Arguments {
public:
    Arguments(int argc, char *argv[]) {
        std::optional<std::string> output;
        std::optional<std::string> input;
        bool emit_hir = false;
        bool emit_asm = false;
        bool print_help = false;
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
            } else if (arg == "-h") {
                print_help = true;
            } else if (startwith("--", arg) || startwith("-", arg)) {
                usage(std::cerr, UsageKind::UnknownOption);
            } else {
                if (input) {
                    usage(std::cerr, UsageKind::DuplicatedInput);
                } else {
                    input.emplace(argv[i]);
                }
            }
        }
        if (!input && !print_help) {
            usage(std::cerr, UsageKind::NoInputFile);
        } else {
            input_ = input ? input.value() : "";
            output_ = output;
            emit_hir_ = emit_hir;
            emit_asm_ = emit_asm;
            print_help_ = print_help;
        }
    }
    const std::string &input() const { return input_; }
    const std::optional<std::string> &output() const { return output_; }
    bool emit_hir() const { return emit_hir_; }
    bool emit_asm() const { return emit_asm_; }
    bool print_help() const { return print_help_; }

private:
    std::string input_;
    std::optional<std::string> output_;
    bool emit_hir_;
    bool emit_asm_;
    bool print_help_;
};

int main(int argc, char *argv[]) {
    Arguments args(argc, argv);
    if (args.print_help()) {
        usage(std::cout, UsageKind::Normal);
    } else if (args.emit_hir()) {
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
            auto success = mini::CodeGenFile(ctx, ofs, args.input());
            if (!success) std::exit(EXIT_FAILURE);
        } else {
            mini::Context ctx;
            auto success = mini::CodeGenFile(ctx, std::cout, args.input());
            if (!success) std::exit(EXIT_FAILURE);
        }
    } else {
        char asm_file[] = "/tmp/mini-XXXXXX.s";
        char obj_file[] = "/tmp/mini-XXXXXX.o";
        std::string output = args.output() ? args.output().value() : "a.out";

        int asm_fd = mkstemps(asm_file, 2);
        if (asm_fd == -1) mini::FatalError("failed to create temporary file");

        int obj_fd = mkstemps(obj_file, 2);
        if (obj_fd == -1) mini::FatalError("failed to create temporary file");

        std::ofstream asm_fs(asm_file);
        if (asm_fs.bad()) {
            mini::FatalError("failed to open output file");
        }

        mini::Context ctx;
        auto success = mini::CodeGenFile(ctx, asm_fs, args.input());
        if (!success) std::exit(EXIT_FAILURE);

        int as_result =
            system(fmt::format("as {} -o {}", asm_file, obj_file).c_str());
        if (as_result) mini::FatalError("as failed");

        int ld_result =
            system(fmt::format("ld {} -o {}", obj_file, output).c_str());
        if (ld_result) mini::FatalError("ld failed");

        close(asm_fd);
        close(obj_fd);
    }
}
