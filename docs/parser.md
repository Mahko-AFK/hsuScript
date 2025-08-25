# Parser

The parser consumes tokens and produces an abstract syntax tree (AST) describing the program.

## Data Structures
- `NodeKind` enumerates all possible AST node types (programs, statements, expressions, literals, etc.).
- `Vec` is a growable array used to hold child nodes for block-like constructs.
- `Node` represents a single AST node with its `kind`, optional token `type`, operator `op`, literal `value`, pointers `left`/`right`, and `children` vector.

## Key Functions
- `parser(Token *tokens)` builds an AST rooted at `NK_Program` from the token stream.
- `init_node` allocates and initializes nodes; `free_tree` recursively releases them.
- The Pratt parser helpers (`parse_expr`, `nud`, and `lbp`) handle expression parsing with proper precedence.
- Statement helpers like `parse_if`, `parse_while`, and `parse_for` build control-flow constructs.

## Example Workflow
```c
Token *toks = lexer(f);
Node *program = parser(toks);
print_tree(program, 0);   // visualize the AST
```

## Extending
To introduce a new AST node:
1. Add a value to `NodeKind` and update `kind_name` for debugging output.
2. Write parsing logic that produces the new node (either in `nud`, `parse_expr`, or a statement helper).
3. Update semantic analysis and code generation to handle the new node kind.
