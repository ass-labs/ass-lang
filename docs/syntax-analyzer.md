# ASS-Lang Syntax Analyzer

This document describes the ASS-Lang syntax analyzer. It covers the grammar it
implements, the two parsers (LL and LR), the derivations and AST they output,
and how to test them.

## Usage

```sh
make
./build/ass run path/to/program.ass
```

After lexical analysis succeeds, the program is parsed and these files are
written next to the source:

| File | Contents |
| --- | --- |
| `<file>.ass.sym` | Symbol table (see [lexical-analyzer.md](lexical-analyzer.md)) |
| `<file>.ass.leftmost` | Leftmost derivation, from the LL parser |
| `<file>.ass.rightmost` | Rightmost derivation, from the LR parser |
| `<file>.ass.ast` | Abstract syntax tree |

- If the lexer reports errors, the program isn't parsed.
- Syntax errors are printed to stderr. The exit code is non-zero, and the
  derivations and AST are not written.
- Which parsers run, and which files are written, is set in `src/config.c`
  (`p_conf` and `out_conf`).

## Pipeline

```
tokens ──► drop comments, append EOF ──┬─► LL parser ─► parse tree ─┐
                                       └─► LR parser ─► parse tree ─┴─► same tree? ─► AST
```

1. Comment tokens are removed and an end-of-file token is appended.
2. The LL parser and the LR parser each build a **parse tree** from the same
   grammar.
3. When both parsers run, their trees must be identical. A difference is a
   parser bug, and the program panics.
4. The leftmost and rightmost derivations are printed from the parse trees.
5. The parse tree is lowered into the **AST** (`include/ast/`).

## Grammar

`{ x }` means zero or more `x`, `[ x ]` means an optional `x`, and quoted
strings are tokens. Statements have no terminator: the language is
free-format, and every statement starts with a distinct token.

```
<program>       ::= <stmt_list>
<stmt_list>     ::= { <stmt> }
<stmt>          ::= <input_stmt> | <output_stmt> | <cond_stmt> | <iter_stmt>
                  | <decl_stmt> | <assign_stmt>
<blk>           ::= "{" { <stmt> } "}"

<input_stmt>    ::= "read" "(" <input_list> ")"
<input_list>    ::= <ident> { "," <ident> }

<output_stmt>   ::= "write" "(" <output_list> ")"
<output_list>   ::= <output_item> { "," <output_item> }
<output_item>   ::= <expr>

<cond_stmt>     ::= "if" "(" <bool_expr> ")" <blk> [ "else" ( <cond_stmt> | <blk> ) ]
<iter_stmt>     ::= "while" "(" <bool_expr> ")" <blk>

<decl_stmt>     ::= [ "let" ] <type> <decl_assign> { "," <decl_assign> }
<decl_assign>   ::= <ident> [ "=" <expr> ]

<assign_stmt>   ::= <ident> <assign_op> <expr>
<assign_op>     ::= "=" | "+=" | "-=" | "*=" | "/=" | "//="

<bool_expr>     ::= <expr>
<expr>          ::= <or_expr>
<or_expr>       ::= <and_expr> { <or_op> <and_expr> }
<and_expr>      ::= <not_expr> { <and_op> <not_expr> }
<not_expr>      ::= <not_op> <not_expr> | <rel_expr>
<rel_expr>      ::= <add_expr> [ <relational_op> <add_expr> ]
<add_expr>      ::= <mul_expr> { <add_op> <mul_expr> }
<mul_expr>      ::= <unary_expr> { <mul_op> <unary_expr> }
<unary_expr>    ::= <unary_op> <pow_expr> | <pow_expr>
<pow_expr>      ::= <primary> [ <pow_op> <unary_expr> ]
<primary>       ::= <ident> | <literal> | "(" <expr> ")"

<or_op>         ::= "OR"
<and_op>        ::= "AND"
<not_op>        ::= "NOT"
<relational_op> ::= ">" | "<" | ">=" | "<=" | "==" | "!="
<add_op>        ::= "+" | "-"
<mul_op>        ::= "*" | "/" | "%" | "//"
<unary_op>      ::= "+" | "-"
<pow_op>        ::= "^"

<type>          ::= "wnum" | "dnum" | "bool" | "char"
<ident>         ::= ( <letter> | "_" ) { <letter> | <digit> | "_" }

<literal>       ::= <wnum_lit> | <dnum_lit> | <bool_lit> | <char_lit> | <string_lit>
<wnum_lit>      ::= <digit> { <digit> }
<dnum_lit>      ::= <digit> { <digit> } "." <digit> { <digit> }
<bool_lit>      ::= "True" | "False"
<char_lit>      ::= "'" <character> "'"
<string_lit>    ::= '"' { <character> } '"'
```

`<letter>`, `<digit>` and `<character>` are the same as in the proposal. The
lexer recognizes identifiers and literals, so the parsers treat them as
single tokens.

### Operator precedence

From highest to lowest. This follows the proposal's precedence table.

| Level | Operators | Associativity |
| --- | --- | --- |
| 1 | `( )` | — |
| 2 | `^` | right (`2 ^ 3 ^ 2` = `2 ^ (3 ^ 2)`) |
| 3 | unary `+` `-` | — (`-x ^ 2` = `-(x ^ 2)`) |
| 4 | `*` `/` `//` `%` | left |
| 5 | `+` `-` | left |
| 6 | `==` `!=` `<` `>` `<=` `>=` | none (`a < b < c` is a syntax error) |
| 7 | `NOT` | — |
| 8 | `AND` | left |
| 9 | `OR` | left |

### Changes from the proposal's grammar

| Change | Reason |
| --- | --- |
| `<assign_op>` added: `+=` `-=` `*=` `/=` `//=` | Compound assignment is now implemented instead of reserved. |
| `"let"` is supported in `<decl_stmt>` | It was in the grammar, but the lexer gave it no category and no derivation used it. `let` is now the noise word `NW_LET`. |
| `^` is now a token | `<pow_op>` was in the grammar, but the lexer rejected `^` as an unrecognized character. |
| `<pow_expr>` is `<primary> [ <pow_op> <unary_expr> ]` | It was `<primary> { <pow_op> <primary> }`, which made `^` left-associative and rejected `2 ^ -1`. |
| `<input_list>` takes `<ident>`s | It took `<expr>`s, which accepted `read(1 + 2)`. |
| `<ident>` may start with `_` | Matches the proposal's identifier rules (`_variable` is valid). |
| No `;` after statements | The final grammar has no terminator. `;` is still a token, but using it is a syntax error. |

## The parsers

Both parsers read the tokens left to right and build the same parse tree.
They differ in which derivation they naturally produce.

### LL parser (top-down) — `src/syntax/ll_parser.c`

A recursive-descent LL(1) parser with one function per nonterminal. It
expands the leftmost nonterminal first, so its tree reads as the **leftmost
derivation**. One token of lookahead is always enough: every statement starts
with a different token, and every optional or repeated part starts with a
token that can't follow it.

### LR parser (bottom-up) — `src/syntax/lr_parser.c`

A table-driven SLR(1) shift-reduce parser. Its reductions, read in reverse,
are the **rightmost derivation**.

- The tables are built from the grammar in `src/syntax/grammar.c` on first
  use. The canonical LR(0) automaton has 133 states, built from 97
  productions, 68 terminals and 52 nonterminals.
- A shift-reduce or reduce-reduce conflict panics with the state and token.
  An ambiguous grammar change is caught the first time anything is parsed.
- LR parsers need BNF, so each `{ ... }` repetition is a *hidden*,
  left-recursive nonterminal, for example `<add_list> ::= <mul_expr> |
  <add_list> <add_op> <mul_expr>`. Hidden nonterminals are spliced into their
  parent in the parse tree. That keeps the tree, and the derivations, flat as
  in the EBNF grammar (`<add_expr> => <mul_expr> <add_op> <mul_expr>`).

### Syntax errors

Both parsers stop at the first syntax error and report it in the lexer's
format, tagged with the parser that found it. Both always report the same
position:

```
e.ass:1:11: syntax error (LL): expected `)`, found `<`
e.ass:1:11: syntax error (LR): expected `AND`, `OR` or `)`, found `<`
```

The LL parser describes what it was parsing ("an expression", "a statement").
The LR parser lists every token that would have been accepted. Candidates
from the SLR table are only listed if the parser would actually shift them,
which is checked by simulating the parse.

| Input | LL error |
| --- | --- |
| `wnum x = 1;` | expected a statement, found `;` |
| `x = (1 + 2` | expected `)`, found end of file |
| `if (x) { } else write(x)` | expected `if` or `{`, found `write` |
| `let x = 1` | expected a type (`wnum`, `dnum`, `bool` or `char`), found `x` |
| `read(1 + 2)` | expected an identifier, found `1` |
| `x == 3` | expected an assignment operator (`=`, `+=`, `-=`, `*=`, `/=` or `//=`), found `==` |
| `wnum8 y = 2` | expected a statement, found `wnum8` |

## Output formats

### Derivations

One sentential form per line, in the same format as the proposal's
derivations. Here is `let wnum x = 123`. The leftmost derivation is on the
left and the rightmost on the right:

```
<program> => <stmt_list>                    <program> => <stmt_list>
          => <stmt>                                   => <stmt>
          => <decl_stmt>                              => <decl_stmt>
          => let <type> <decl_assign>                 => let <type> <decl_assign>
          => let wnum <decl_assign>                   => let <type> <ident> = <expr>
          => let wnum <ident> = <expr>                => let <type> <ident> = <or_expr>
          => let wnum x = <expr>                      => ...
          => let wnum x = <or_expr>                   => let <type> <ident> = <wnum_lit>
          => ...                                      => let <type> <ident> = 123
          => let wnum x = <wnum_lit>                  => let <type> x = 123
          => let wnum x = 123                         => let wnum x = 123
```

Derivations stop at tokens. The proposal expands `<ident> => <letter> => x`
and `<wnum_lit> => <digit> <digit> => 6 7`, but those steps belong to the
lexer, so here they are `<ident> => x` and `<wnum_lit> => 67`.

### AST

The AST is written as an indented S-expression. Binary and unary expressions
are `(op operands...)`, so the grouping is explicit:

```
(program
  (decl wnum
    (count 0)
    total)
  (decl bool
    (done (OR (AND (NOT False) (< count 3)) True)))
  (write "count = " count (- (^ count (^ 2 3))))
  (assign count += 1)
  (if (> count 0)
    (block
      (write "pos"))
    (block
      (if (< count 0)
        (block
          (write "neg"))
        (block
          (write "zero"))))))
```

| Node | AST | Fields (`include/ast/`) |
| --- | --- | --- |
| Declaration | `(decl <type> x (y <expr>))` | `ty`, `items` (name, optional value) |
| Assignment | `(assign x <op> <expr>)` | `target`, `op`, `val` |
| Conditional | `(if <cond> (block ...) [(block ...)])` | `cond`, `consequent`, `alternate`. An `else if` is an `alternate` block holding the nested `if`. |
| Loop | `(while <cond> (block ...))` | `cond`, `body` |
| Input | `(read a b)` | `targets` |
| Output | `(write <expr> ...)` | `vals` |
| Expressions | literal, identifier, `(op x)`, `(op x y)` | `EXPR_LITERAL`, `EXPR_IDENT`, `EXPR_UNARY`, `EXPR_BINARY` |

`let` and parentheses don't appear in the AST. They only matter to the syntax.

## Test sources

`tests/src/syntax/` has one file per derivation case in the proposal, plus
the new rules. Run any of them with `./build/ass run <file>` and compare the
`.leftmost` and `.rightmost` outputs with the proposal's derivations.

| Statement | Files |
| --- | --- |
| Input | `input_single`, `input_multiple` |
| Output | `output_string`, `output_number`, `output_variable`, `output_multiple` |
| Declaration | `decl_no_value`, `decl_with_value`, `decl_multiple_unassigned`, `decl_multiple_assigned`, `decl_mixed`, `decl_let` *(new)* |
| Assignment | `assign_literal`, `assign_arithmetic`, `assign_unary_power`, `assign_logical`, `assign_compound` *(new)* |
| Conditional | `cond_if`, `cond_if_else`, `cond_if_else_if` |
| Loop | `iter_while`, `iter_nested` |
| Everything | `program` — every statement, operator and literal kind |

The outputs are generated, so they're ignored by git, like `.sym` files.

## Changing the grammar

1. Add the productions to `PRODUCTIONS` in `src/syntax/grammar.c`. Add new
   nonterminals to `nonterminal_t` in `include/syntax/grammar.h` and to
   `NT_NAMES`. Repetitions need a hidden nonterminal, declared after
   `____nt_hidden_start`.
2. Add the matching function or branch to `src/syntax/ll_parser.c`, building
   the same children as the production.
3. Lower the new node in `src/syntax/lower.c`, and add an AST node to
   `include/ast/` if needed.
4. Run a test program. The LR tables report conflicts, and the LL and LR
   trees are compared on every run, so a mismatch between the two parsers is
   caught immediately.
