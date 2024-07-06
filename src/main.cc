#include "context.h"
#include "parser.h"

int main() {
    mini::Context ctx;
    auto ds = mini::parse_file(ctx, "main.mini");
    if (!ds) return EXIT_FAILURE;
}
