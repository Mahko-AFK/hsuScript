# Semantic Analysis

The semantic analyser performs type checking and scope resolution on the AST.

## Data Structures
- `TypeKind` defines the primitive types (`TY_INT`, `TY_STRING`, `TY_BOOL`, `TY_VOID`, `TY_UNKNOWN`).
- `Type` is a simple wrapper around `TypeKind` used to annotate AST nodes.
- `Binding` links an identifier name to its `Type` within a scope.
- `Scope` forms a linked list of lexical scopes, each containing a chain of bindings.

## Key Functions
- `sem_expr` infers and validates the type of an expression node.
- `sem_block` walks a block, managing a new scope and checking contained statements.
- `sem_program` initializes the global scope and checks all top-level blocks.
- Scope utilities (`scope_new`, `scope_lookup`, `scope_insert`) manage symbol tables.
- Helper constructors like `type_int`, `type_string`, etc. provide singleton type objects.

## Example Workflow
```c
Node *program = parser(toks);
sem_program(program);   // annotates nodes and reports semantic errors
```

## Extending
When adding new language features:
1. Extend the type system if necessary by updating `TypeKind` and helper constructors.
2. Teach `sem_expr` or `sem_block` how to validate the new AST node kinds.
3. Ensure corresponding changes exist in the parser and code generator so that nodes receive the expected type information.
