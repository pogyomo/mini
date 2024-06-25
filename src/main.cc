#include "context.h"
#include "lexer.h"

int main() {
    Context ctx;
    auto ts = lex_file(ctx, "main.mini");
}
