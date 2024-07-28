# Mini

A minimal, but self-hostable programming language.

## Entry point of program

The `main` function should contained on a executable program, and the program start from the function.

```
function main() {
    let a: usize = 10;
    let b: usize = 20;
    a + b;
}
```

## Types

The mini programming language support a few types described below:

| type    | description                     |
| ------- | ------------------------------- |
| void    | an empty, zero sized type       |
| isize   | signed native size integer      |
| int8    | signed 8-bit integer            |
| int16   | signed 16-bit integer           |
| int32   | signed 32-bit integer           |
| int64   | signed 64-bit integer           |
| usize   | unsigned native size integer    |
| uint8   | unsigned 8-bit integer          |
| uint16  | unsigned 16-bit integer         |
| uint32  | unsigned 32-bit integer         |
| uint64  | unsigned 64-bit integer         |
| char    | 8-bit value for ascii codepoint |
| bool    | 0 for false or 1 for true       |
| struct  | a collection of value           |
| enum    | a collection of unique ints     |
| array   | a list of value                 |
| pointer | a value pointing to a value     |

## Integer literal

The type of integer literal is determined by its size. More precisly, if the integer cannot be represented by n-bit, but can be by m-bit, where n < m and n \* 2 = m, the type will be uint**m**

If the integer cannot be represented by 64-bit, this cause compile error.

## Implicit integer conversion

An integer will be converted to target integer type implicitly with following rule:

- If the integer is unsigned:
  - If target integer type is unsigned or signed, then convertion always success if target size is larger than or equal to original one.
  - Otherwise the convertion always fail.
- If the integer is signed.
  - If target integer type is signed, then convertion always success if target size is larger than or equal to original one.
  - Otherwise the convertion always fail.

Here is the examples of implicit convertion at arthmetic operations:

- Integers will be converted for infix operator when two integers has same sign, and the size rounded into bigger one.

```
let a: uint8 = 10;
let b: uint16 = 20;
let c: uint16 = a + b;
```

- If unsigned integer was negated, it implicitly converted into signed one with bigger size.

```
let a: uint8 = 10;
let b: int16 = -a;
```

- The conversion for two integer with different sign changes the sign of result.

```
let a: uint8 = 10;
let b: int8 = -10;
let c: int16 = a + b;
```

## Function

Functions cannot accept value which size is more thant 8 byte, and cannot accept more than 6 arguments.

Also, function must returns value less than or equal to 8 byte.

## Constant Expression

A constant expression is an expression which contains only integer literal and using only some arthemtic operator, and evaluated as `usize`.

Infix operators `&, |, ^, <<, >>, +, -, *, /, %` and unary operator `~` is allowed.

## Syntax

The syntax of mini programming language is as follow:

```
<program> ::= { <declaration> }*

<declaration> ::= <function-declaration>
                | <struct-declaration>
                | <enum-declaration>
<function-declaration> ::= "function" "(" <function-parameters> ")" [ "->" <type> ] <block-statement>
<function-parameters> ::= <function-parameter>
                        | <function-parameter> "," <function-parameters>
<function-parameter> ::= <identifier> ":" <type>
<struct-declaration> ::= "struct" "{" <struct-items> "}"
<struct-items> ::= <struct-item> [ "," [ <struct-items> ] ]
<struct-item> ::= <identifier> ":" <type>
<enum-declaration> ::= "enum" "{" <enum-items> "}"
<enum-items> ::= <enum-item> [ "," [ <enum-items> ] ]
<enum-item> ::= <identifier> [ "=" <constant-expression> ]

<type> ::= "void"
         | "isize"
         | "int8"
         | "int16"
         | "int32"
         | "int64"
         | "usize"
         | "uint8"
         | "uint16"
         | "uint32"
         | "uint64"
         | "char"
         | "bool"
         | <struct-or-enum-name>
         | <array>
         | <pointer>
<struct-or-enum-name> :: <identifier>
<array> ::= "(" <type> ")" <array-indexe>
<array-index> ::= "[" [ <constant-expression> ] "]"
<pointer> ::= "*" <type>

<statement> ::= <expression-statement>
              | <return-statement>
              | <break-statement>
              | <continue-statement>
              | <while-statement>
              | <if-statement>
              | <block-statement>
<expression-statement> ::= <expression> ";"
<return-statement> ::= "return" [ <expression> ] ";"
<break-statement> ::= "break" ";"
<continue-statement> ::= "continue" ";"
<while-statement> ::= "while" "(" <expression> ")" <statement>
<if-statement> ::= "if" "(" <expression> ")" <statement> [ "else" <statement> ]
<block-statement> ::= "{" <block-statement-items> "}"
<block-statement-items> ::= <block-statement-item> [ <block-statement-items> ]
<block-statement-item> ::= <variable-declarations> | <statement>
<variable-declarations> ::= "let" <variable-declarations-ids> ";"
<variable-declarations-ids> ::= <identifier> ":" <type> [ "=" <expression> ] [ <variable-declarations-ids> ]

<constant-expression> ::= <logical-or-expression>
<expression> ::= <logical-or-expression>
               | <unary-expression> "=" <expression>
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
                    | <cast-expression> "as" <type>
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
                       | <string>
                       | <character>
                       | <struct-init>
                       | <array-init>
                       | "true"
                       | "false"
                       | "(" <expression> ")"
<enum-select> ::= <identifier> "::" <identifier>
<integer> ::= ...
<string> ::= ...
<character> ::= ...
<struct-init> ::= <identifier> "{" <struct-initializers> "}"
<struct-initializers> ::= <struct-initializer> [ "," [ <struct-initializers> ] ]
<struct-initializer> ::= <identifier> ":" <expression>
<array-init> ::= "[" <array-initializers> "]"
<array-initializers> ::= <expression> [ "," [ <array-initializers> ] ]
```
