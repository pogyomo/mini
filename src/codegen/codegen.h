#ifndef MINI_CODEGEN_CODEGEN_H_
#define MINI_CODEGEN_CODEGEN_H_

#include <ostream>
#include <string>

#include "../context.h"

namespace mini {

bool CodeGenFile(Context &ctx, std::ostream &os, const std::string &path);

}

#endif  // MINI_CODEGEN_CODEGEN_H_
