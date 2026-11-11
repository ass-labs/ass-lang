# ASS-Lang Lexical Analyzer

This document describes the tokens the ASS-Lang lexical analyzer recognizes.
It covers what was added to bring the lexer in line with the language proposal
(Group 6, *Applied Student-Oriented Scripting Language*).

## Usage

```sh
make
./build/ass run path/to/program.ass
```

- Only files ending in `.ass` are accepted. Any other file is rejected before
  lexing starts.
- The symbol table is written next to the source as `<file>.ass.sym`, a plain
  text file.
- Lexical errors are printed to stderr as `file:row:col: error: <reason>`.
  Analysis continues past errors, and the exit code is non-zero if any error
  was found.

## Symbol table format

Each token is one row with four columns:

```
ROW:COL    CATEGORY       KIND             LEXEME
7:1        KEYWORD        KW_WNUM          wnum
7:6        IDENTIFIER     IDENT            age
2:1        COMMENT        BLOCK_COMMENT    @$$\nGroup 6 sample program\n@$$
53:6       INVALID        UNKNOWN          123variable
```

- **CATEGORY** (new) groups each token by lexical class: `IDENTIFIER`,
  `KEYWORD`, `RESERVED_WORD`, `CONSTANT`, `ARITH_OP`, `RELATIONAL_OP`,
  `LOGICAL_OP`, `ASSIGN_OP`, `RESERVED_OP`, `DELIMITER`, `BRACKET`, `COMMENT`
  or `INVALID`.
- Newlines, carriage returns and tabs inside a lexeme are written as `\n`,
  `\r` and `\t`. This keeps multi-line comments on a single row.

## Tokens

### Keywords

| Lexeme | Kind | Description |
| --- | --- | --- |
| `wnum` | `KW_WNUM` | Integer data type *(new)* |
| `dnum` | `KW_DNUM` | Double data type *(new)* |
| `char` | `KW_CHAR` | Character data type *(new)* |
| `bool` | `KW_BOOL` | Boolean data type *(new)* |
| `write` | `KW_WRITE` | Output statement *(new)* |
| `read` | `KW_READ` | Input statement *(new)* |
| `if` / `else` / `while` | `KW_IF` / `KW_ELSE` / `KW_WHILE` | Control flow |
| `AND` / `OR` / `NOT` | `KW_AND` / `KW_OR` / `KW_NOT` | Logical operators (category `LOGICAL_OP`) |
| `True` / `False` | `KW_TRUE` / `KW_FALSE` | Boolean constants (category `CONSTANT`) |

Keywords are case-sensitive, so `If`, `WHILE` and `true` are identifiers. A
keyword is only matched as a whole word. For example, `wnumx`, `writer` and
`boolean` are identifiers.

### Reserved words *(new)*

These words are reserved for future features. The lexer recognizes them so
they can't be used as identifiers, but the language doesn't give them any
meaning yet.

| Lexeme | Kind | Future feature |
| --- | --- | --- |
| `wnum8` `wnum16` `wnum32` `wnum64` | `RW_WNUM8` … `RW_WNUM64` | Explicit integer bit widths |
| `dnum8` `dnum16` `dnum32` `dnum64` | `RW_DNUM8` … `RW_DNUM64` | Explicit double bit widths |
| `auto` / `let` | `RW_AUTO` / `RW_LET` | Automatic type deduction |
| `str` | `RW_STR` | String type |

Unlisted widths such as `wnum7` and `wnum128` are ordinary identifiers.

### Identifiers

The pattern is `[A-Za-z_][A-Za-z0-9_]*`, excluding keywords and reserved words.

**New:** a number followed directly by identifier characters, such as
`123variable` or `1.5abc`, is reported as a single invalid token. The error is
*identifiers cannot start with a digit*. Before this change, it was split into
a number and an identifier.

### Constants

| Kind | Pattern | Examples |
| --- | --- | --- |
| `INT` | `<digit>+` | `0`, `94` |
| `FLOAT` | `<digit>+ . <digit>+` | `15.05`, `0.123` |
| `CHAR` | `'` one character or escape `'` | `'A'`, `'\n'`, `'\0'` |
| `STR` | `"` characters or escapes `"` on one line | `"Passed"` |
| `KW_TRUE` / `KW_FALSE` | `True` / `False` | — |

The supported escapes are `\n`, `\t`, `\r`, `\0`, `\\`, `\'` and `\"`.

### Operators

| Group | Lexemes | Kinds |
| --- | --- | --- |
| Arithmetic | `+` `-` `*` `/` `%` `//` | `PLUS` `MINUS` `STAR` `SLASH` `PERCENT` `SLASH_SLASH` |
| Assignment *(new)* | `=` | `EQ` |
| Relational *(new)* | `==` `!=` `<` `>` `<=` `>=` | `EQ_EQ` `BANG_EQ` `LT` `GT` `LT_EQ` `GT_EQ` |
| Logical | `AND` `OR` `NOT` | `KW_AND` `KW_OR` `KW_NOT` |
| Reserved compound assignment *(new)* | `+=` `-=` `*=` `/=` `//=` | `PLUS_EQ` `MINUS_EQ` `STAR_EQ` `SLASH_EQ` `SLASH_SLASH_EQ` |

`//` was declared before this change but never lexed. It now works.

Operators follow the longest match: `a//b` lexes as `a`, `//`, `b`, and `<=` is
one token, not `<` followed by `=`. A lone `!` is invalid because `!` is only
valid as part of `!=`.

### Delimiters and brackets

| Category | Lexemes |
| --- | --- |
| `DELIMITER` | `;` `,` `.` |
| `BRACKET` | `(` `)` `{` `}` `[` `]` |

### Comments *(new)*

Comments are recognized and written to the symbol table under the `COMMENT`
category. They will be discarded by the parser.

| Kind | Syntax | Rule |
| --- | --- | --- |
| `LINE_COMMENT` | `$$ text` | Runs from `$$` to the end of the line |
| `BLOCK_COMMENT` | `@$$ text @$$` | Can span lines and ends at the first closing `@$$` |

```
$$ This is a comment
wnum x = 1; $$ trailing comment
@$$
This is a multi-line comment block
for documentation purposes
@$$
wnum y = 2; @$$ inline block @$$ wnum z = 3;
```

Edge cases:

- `$$$ text` is a line comment.
- `@$$@$$` is an empty block comment.
- A lone `$`, a lone `@` or `@$` is an unrecognized character.
- A `@$$` that is never closed is reported once as *unterminated multi-line
  comment*. The error covers the rest of the file.

### Invalid tokens

Any lexeme that matches no rule becomes an `UNKNOWN` token with category
`INVALID`, and an error is reported. Examples:

| Input | Reason |
| --- | --- |
| `@` `#` `` ` `` `~` `?` `:` `\` `^` `&` `\|` `!` `$` | unrecognized character |
| `123variable` | identifiers cannot start with a digit |
| `"no closing quote` | unterminated or malformed string literal |
| `'ab'` | unterminated or malformed character literal |
| `@$$ ...` (never closed) | unterminated multi-line comment |

### Noise words

The proposal defines no noise words, so the lexer doesn't recognize any.

## Implementation notes

All tokens are recognized by a single hand-coded DFA in `src/lexical/dfa.c`.
Its states are the `dfa_state_t` enum in `include/lexical/dfa.h`. The
transitions are `switch` statements in `dfa_step`, and `dfa_accepts` maps each
accepting state to its token kind. `lexer_munch` in `src/lexical/analyzer.c`
runs the DFA with maximal munch.

- **Keywords and reserved words**: each word is a path of states that
  branches off the start state, one state per prefix (`DFA_W` → `DFA_WH` →
  … → `DFA_WHILE`). The intermediate states accept `IDENT`, and the last state
  accepts the word's kind. On any other identifier character a path leaves to
  `DFA_IDENT`, so `whilst` and `ifx` are identifiers.
- **Symbols**: multi-character operators share their prefix states, so `/`,
  `//`, `/=` and `//=` all start at `DFA_SLASH`. A prefix that isn't a symbol
  on its own, such as `!`, stays non-accepting.
- **Line comments**: `$` → `$` → an accepting body that loops on every byte
  except `\r` and `\n`.
- **Block comments**:

  ```
  () -@-> () -$-> () -$-> (body) -@-> (@) -$-> (@$) -$-> ((end))
                           ^  ^        | ^       |
                           |  +-[^@$]--+ +---@---+
                           +----------[^@$]------+
  ```

- **Bad identifiers**: `DFA_INT` and `DFA_FLOAT` go to the accepting
  `DFA_BAD_IDENT` state (kind `UNKNOWN`) on any identifier character.
  `lexer_analyze` reports it as an error.

To add a keyword, reserved word or operator:

1. Add its kind in `include/lexical/token.h`. Keywords and reserved words go
   between the matching `____tok_*_start` and `____tok_*_end` sentinels.
2. Give it a spelling in `TOKEN_KIND_STRS` and a name in `TOKEN_KIND_NAMES`.
   Operators also need an entry in `token_kind_category`.
3. Add a state for each new prefix to `dfa_state_t`, add the transitions to
   `dfa_step`, and map the final state to the kind in `dfa_accepts`. Prefixes
   of keywords must also be listed as accepting `TOK_IDENT`.

## Test sources

| File | Covers |
| --- | --- |
| `tests/src/sample.ass` | Every token in the proposal, plus invalid lexemes |
| `tests/src/lexical/keywords.ass` | Keywords, reserved words and near-miss identifiers |
| `tests/src/lexical/operations.ass` | Arithmetic, relational, assignment and compound operators |
| `tests/src/lexical/comments.ass` | Line and block comments and their edge cases |
| `tests/src/lexical/literals.ass` | Constants |
| `tests/src/lexical/delims.ass` | Delimiters and brackets |
