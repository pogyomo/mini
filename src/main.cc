#include "context.h"
#include "parser.h"

int main() {
    Context ctx;
    auto ds = parse_file(ctx, "main.mini");
}
