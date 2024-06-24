# Mini

A minimal, but self-hostable programming language.

## Entry point of program

The `main` function should contained on a executable program, and the program start from the function.

```
function main() -> int {
    
    let a = 10, b = 20: int;
    return a + b;
}
```

You can take a arguments and use it in usr program as follow:

```
function main(argc: int, argv: *(int)[]) -> int {
    if (argc < 2) return 0;
    return argv[0][0];
}
```

All program must uses either of above two signature.

## Types

The mini programming language support a few types described below:

| type    | description                 |
+---------+-----------------------------+
| int     | signed 64-bit integer       |
| uint    | unsigned 64-bit integer     |
| struct  | a collection of value       |
| enum    | a collection of unique ints |
| array   | a list of value             |
| pointer | a value pointing to a value |

## Syntax

The syntax of mini programming language is as follow:

```
<program> ::= { <declaration> }*

<declaration> ::= <function-declaration>
                | <struct-declaration>
                | <enum-declaration>
<function-declaration> ::= "function" "(" <function-parameters> ")" "->" <type> <block-statement>
<function-parameters> ::= <function-parameter>
                        | <function-parameter> "," <function-parameters>
<function-parameter> ::= <identifier> ":" <type>
<struct-declaration> ::= "struct" "{" <struct-items> "}"
<struct-items> ::= <struct-item> [ "," <struct-items> ]
<struct-item> ::= <identifier> ":" <type>
<enum-declaration> ::= "enum" "{" <enum-items> "}"
<enum-items> ::= <enum-item> [ "," <enum-items> ]
<enum-item> ::= <identifier>

<type> ::= "int"
         | "uint"
         | <struct-or-enum-name>
         | <array>
         | <pointer>
<struct-or-enum-name> :: <identifier>
<array> ::= "(" <type> ")" <array-indexes>
<array-indexes> ::= <array-index> [ <array-indexes> ]
<array-index> ::= "[" <const-expression> "]"
<pointer> ::= "*" <type>

<statement> ::= <expression-statement>
              | <return-statement>
              | <break-statement>
              | <continue-statement>
              | <while-statement>
              | <if-statement>
              | <block-statement>
<expression-statement> ::= <expression> ";"
<return-statement> ::= "return" <expression> ";"
<break-statement> ::= "break" ";"
<continue-statement> ::= "continue" ";"
<while-statement> ::= "while" "(" <expression> ")" <statement>
<if-statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]
<block-statement> ::= "{" <block-statement-items> "}"
<block-statement-items> ::= <block-statement-item> [ <block-statement-items> ]
<block-statement-item> ::= { <variable-declaration> }* { <statement> }*
<variable-declaration> ::= "let" <variable-declaration-ids> ";"
<variable-declaration-ids> ::= <identifier> [ "=" <expression> ] [ <variable-declaration-ids> ]

<expression> ::= <logical-or-expression>
<logical-or-expression> ::= <logical-and-expression> [ "||" <logical-or-expression> ]
<logical-and-expression> ::= <inclusive-or-expression> [ "&&" <logical-and-expression> ]
<inclusive-or-expression> ::= <exclusive-or-expression> [ "|" <inclusive-or-expression> ]
<exclusive-or-expression> ::= <and-expression> [ "^" <exclusive-or-expression> ]
<and-expression> ::= <equality-expression> [ "&" <and-expression> ]
<equality-expression> ::= <relational-expression> [ { "==" | "!=" } <equality-expression> ]
<relational-expression> ::= <shift-expression> [ { "<" | ">" | "<=" | ">=" } <relational-expression> ]
<shift-expression> ::= <additive-expression> [ { "<<" || ">>" } <shift-expression> ]
<additive-expression> ::= <multiplicative-expression> [ { "+" | "-" } <additive-expression> ]
<multiplicative-expression> ::= <cast-expression> [ { "*" | "/" | "%" } <multiplicative-expression> ]
<cast-expression> ::= <unary-expression>
                    | "(" <type> ")" <cast-expression>
<unary-expression> ::= <postfix-expression>
                     | { "&" | "*" | "-" | "~" | "!" } <unary-expression>
                     | "esizeof" <expression>
                     | "tsizeof" <type>
<postfix-expression> ::= <primary-expression>
                       | <postfix-expression> "[" <expression> "]"
                       | <postfix-expression> "(" <function-call-args> ")"
                       | <postfix-expression> "." <identifier>
<function-call-args> ::= <expression> [ "," <function-call-args> ]
<primary-expression> ::= <enum-select>
                       | <identifier>
                       | <integer>
                       | "(" <expression> ")"
<enum-select> ::= <identifier> "::" <identifier>
<integer> ::= ...
```
