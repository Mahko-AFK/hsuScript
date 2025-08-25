# Lexer

The lexer turns raw source code into a linear stream of tokens that the parser can consume.

## Data Structures
- `TokenType` enum categorizes lexemes (identifiers, literals, operators, keywords, etc.).
- `Token` holds a token's `type`, string `value`, and originating `line_num`.

## Key Functions
- `lexer(FILE *file)` reads a file and returns a dynamically sized array of `Token` ending in `END_OF_TOKENS`.
- `generate_number`, `generate_keyword_or_identifier`, `generate_string_token`, and `generate_separator_or_operator` create tokens for specific lexeme classes.
- `print_token` is a debugging helper that displays token information.

## Example Workflow
```c
FILE *f = fopen("program.hsc", "r");
Token *toks = lexer(f);
for (size_t i = 0; toks[i].type != END_OF_TOKENS; i++) {
    print_token(toks[i]);
}
```

## Extending
To support a new lexical feature:
1. Add the token to `TokenType`.
2. Teach the main loop in `lexer.c` how to recognize it (possibly adding a new `generate_*` helper).
3. Update the parser and later compiler stages to understand the new token.
