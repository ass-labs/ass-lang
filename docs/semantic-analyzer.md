# ASS-Lang Semantic Analyzer

The semantic analyzer checks the meaning of a program that parsed without
syntax errors. It walks the AST (`include/ast/`) once, keeping a stack of
scopes, and reports errors and warnings to stderr.

```
tokens ──► parsers ──► AST ──► semantic analyzer ──► errors / warnings
```

- Errors make `ass run` exit with a non-zero code. Warnings don't.
- Diagnostics are printed sorted by position, with an optional `note` line
  pointing at a related place (e.g. the previous declaration).
- Code: `src/semantic/analyzer.c` (checks), `src/semantic/types.c` (types),
  `src/semantic/diagnostic.c` (collecting and printing diagnostics).
- Tests: `tests/src/semantic/*.ass`. `valid.ass` must produce no output, the
  other files each show one kind of check.

## Types

| Type | Values | Literal |
| --- | --- | --- |
| `wnum` | whole numbers | `42` |
| `dnum` | decimal numbers | `4.2` |
| `bool` | `True`, `False` | `True` |
| `char` | one character | `'a'` |
| string | text, only in `write(...)` | `"hi"` |

There is no string variable type (`str` is reserved), so a string literal can
only be written.

The only implicit conversion is `wnum` to `dnum` (e.g. `dnum d = 1`), which
loses nothing. Storing a `dnum` in a `wnum` is an error.

### Operators

| Operators | Operands | Result |
| --- | --- | --- |
| `+` `-` `*` `%` `//` `^` | `wnum`/`dnum` | `dnum` if an operand is a `dnum`, else `wnum` |
| `/` | `wnum`/`dnum` | always `dnum` (true division; `//` is floor division) |
| unary `+` `-` | `wnum`/`dnum` | the operand's type |
| `<` `>` `<=` `>=` | two numbers, or two `char`s | `bool` |
| `==` `!=` | two values of the same type, or two numbers | `bool` |
| `AND` `OR` | `bool` | `bool` |
| `NOT` | `bool` | `bool` |

Compound assignment `x op= v` is checked as `x = x op v`, so `wnum x` with
`x /= 2` is an error (use `//=`).

The conditions of `if` and `while` must be `bool`: `if (x)` with `wnum x` is
an error, write `if (x != 0)`.

## Checks

| Check | Severity | Example |
| --- | --- | --- |
| Type mismatch in a declaration or assignment | error | `wnum w = 1.5` |
| Operator applied to the wrong types | error | `True + 1`, `'a' < 3` |
| Non-`bool` condition | error | `while (n) { ... }` |
| Undeclared variable (read, assigned or `read(...)`) | error | `write(y)` without `y` declared |
| Redefinition in the same scope | error | `wnum x` twice in one block |
| Shadowing a variable of an enclosing scope | warning | `wnum x` inside an `if` when `x` is declared outside |
| Unused variable (never read nor assigned) | warning | `wnum tax` and nothing else |
| Variable assigned but never read | warning | `read(age)` and `age` is never used |
| Use of a variable no path initializes | error | `wnum a` then `write(a)` |
| Use of a variable only some paths initialize | warning | assigned only in an `if` without `else` |

Prefixing a name with `_` (e.g. `_unused`) silences the unused-variable
warnings for it.

### Scopes

The program is the outermost scope, and every block (`{ ... }`) opens a new
one. A variable is visible from its declaration to the end of its block. In
`wnum x = x + 1`, the `x` on the right is not the variable being declared.

### Initialization

The analyzer tracks, at each point, which variables are initialized on every
path (definitely) and on at least one path (possibly):

- A declaration with a value, an assignment and `read(x)` initialize `x`.
- After `if`/`else`, a variable is definitely initialized only if both
  branches initialize it. An `if` without `else` initializes nothing for sure.
- A `while` body may not run, so what it assigns is only possibly initialized
  after the loop. Inside the body, a variable the body assigns later may
  already hold a value from the previous iteration, so it is a warning there,
  not an error.

Reading a variable that is not definitely initialized is an error if no path
initializes it, and a warning otherwise. Each variable is reported once.
`x += 1` reads `x`, so it needs `x` to be initialized, but it doesn't count as
using `x`.
