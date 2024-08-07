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
    os << "  -c          Output object file" << std::endl;
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

static std::string replace_suffix(const std::string &s,
                                  const std::string &suffix) {
    auto pos = s.find_last_of('.');
    if (pos == std::string::npos) return s;
    std::string result(s.begin(), s.begin() + pos + 1);
    result.insert(result.end(), suffix.begin(), suffix.end());
    return result;
}

class Arguments {
public:
    Arguments(int argc, char *argv[]) {
        std::optional<std::string> output;
        std::optional<std::string> input;
        bool emit_hir = false;
        bool emit_asm = false;
        bool emit_obj = false;
        bool print_help = false;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "--emit-hir") {
                if (emit_asm || emit_obj) {
                    mini::FatalError("cannot use --emit-hir with -S and -c");
                }
                emit_hir = true;
            } else if (arg == "-S") {
                if (emit_hir || emit_obj) {
                    mini::FatalError("cannot use -S with --emit-hir and -c");
                }
                emit_asm = true;
            } else if (arg == "-c") {
                if (emit_asm || emit_asm) {
                    mini::FatalError("cannot use -c with --emit-hir and -S");
                }
                emit_obj = true;
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
            emit_obj_ = emit_obj;
            print_help_ = print_help;
        }
    }
    const std::string &input() const { return input_; }
    const std::optional<std::string> &output() const { return output_; }
    bool emit_hir() const { return emit_hir_; }
    bool emit_asm() const { return emit_asm_; }
    bool emit_obj() const { return emit_obj_; }
    bool print_help() const { return print_help_; }

private:
    std::string input_;
    std::optional<std::string> output_;
    bool emit_hir_;
    bool emit_asm_;
    bool emit_obj_;
    bool print_help_;
};

static void gen_hir(const std::string &input, const std::string &output) {
    mini::Context ctx;
    auto root = mini::HirGenFile(ctx, input);
    if (!root) std::exit(EXIT_FAILURE);

    std::ofstream ofs(output);
    if (ofs.bad()) mini::FatalError("failed to open output file");

    mini::hir::PrintableContext pctx(ofs, 4);
    root->PrintLn(pctx);
}

static void gen_asm(const std::string &input, const std::string &output) {
    std::ofstream ofs(output);
    if (ofs.bad()) mini::FatalError("failed to open output file");

    mini::Context ctx;
    auto success = mini::CodeGenFile(ctx, ofs, input);
    if (!success) std::exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    Arguments args(argc, argv);
    if (args.print_help()) {
        usage(std::cout, UsageKind::Normal);
    } else if (args.emit_hir()) {
        std::string output = args.output()
                                 ? args.output().value()
                                 : replace_suffix(args.input(), "hir");
        gen_hir(args.input(), output);
    } else if (args.emit_asm()) {
        std::string output = args.output() ? args.output().value()
                                           : replace_suffix(args.input(), "s");
        gen_asm(args.input(), output);
    } else {
        char asm_file[] = "/tmp/mini-XXXXXX.s";
        char obj_file[] = "/tmp/mini-XXXXXX.o";
        std::string output;
        if (args.output()) {
            output = args.output().value();
        } else if (args.emit_obj()) {
            output = replace_suffix(args.input(), "o");
        } else {
            output = "a.out";
        }

        int asm_fd = mkstemps(asm_file, 2);
        if (asm_fd == -1) mini::FatalError("failed to create temporary file");

        int obj_fd = mkstemps(obj_file, 2);
        if (obj_fd == -1) mini::FatalError("failed to create temporary file");

        gen_asm(args.input(), asm_file);

        if (args.emit_obj()) {
            int as_result =
                system(fmt::format("as {} -o {}", asm_file, output).c_str());
            if (as_result) {
                close(asm_fd);
                close(obj_fd);
                mini::FatalError("as failed");
            }
        } else {
            char start_asm_file[] = "/tmp/mini-XXXXXX.s";
            char start_obj_file[] = "/tmp/mini-XXXXXX.o";
            int start_asm_fd = mkstemps(start_asm_file, 2);
            if (start_asm_fd == -1)
                mini::FatalError("failed to create temporary file");
            int start_obj_fd = mkstemps(start_obj_file, 2);
            if (start_obj_fd == -1)
                mini::FatalError("failed to create temporary file");

            std::ofstream start(start_asm_file);
            start << "    .text" << std::endl;
            start << "    .global _start" << std::endl;
            start << "_start:" << std::endl;
            start << "    callq main" << std::endl;
            start << "    movq %rax, %rdi" << std::endl;
            start << "    movq $60, %rax" << std::endl;
            start << "    syscall" << std::endl;

            int as_result = system(
                fmt::format("as {} -o {}", start_asm_file, start_obj_file)
                    .c_str());
            if (as_result) {
                close(asm_fd);
                close(obj_fd);
                close(start_asm_fd);
                close(start_obj_fd);
                mini::FatalError("as failed");
            }

            as_result =
                system(fmt::format("as {} -o {}", asm_file, obj_file).c_str());
            if (as_result) {
                close(asm_fd);
                close(obj_fd);
                close(start_asm_fd);
                close(start_obj_fd);
                mini::FatalError("as failed");
            }

            int ld_result = system(
                fmt::format("ld -dynamic-linker "
                            "/lib64/ld-linux-x86-64.so.2 -lc {} {} -o {}",
                            obj_file, start_obj_file, output)
                    .c_str());

            close(start_asm_fd);
            close(start_obj_fd);
            if (ld_result) {
                close(asm_fd);
                close(obj_fd);
                mini::FatalError("ld failed");
            }
        }

        close(asm_fd);
        close(obj_fd);
    }
}
