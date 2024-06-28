#include <iostream>

#include "codegen/codegen.h"
#include "context.h"
#include "parser.h"

int main() {
    Context ctx;
    auto ds = parse_file(ctx, "main.mini");
    if (!ds) return EXIT_FAILURE;
    codegen(ctx, std::cout, *ds);
}
