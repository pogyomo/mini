# HIR - Higher Intermediate Representation

A kind of representation of code which is easy to understand for compiler.

The difference between AST and HIR is as follow:

## Simple

AST holds **all** infomation of source code: include comma, parenthesis, etc.

These infomation is not used anymore after parsing, HIR only holds necessary infomation.

## Variable Declaration

Instead of AST holds variable declarations for each block, HIR holds **all** declaration in `hir::FunctionDeclaration`.

For this, compiler easily calculate and optimize the size of local variables.
