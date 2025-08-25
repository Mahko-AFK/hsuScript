# Code Generation

The code generator turns the typed AST into x86-64 assembly.

## Data Structures
- `Symbol` records a variable's name, stack-frame `offset`, and whether it holds a string.
- `CGScope` is a stack of symbol tables mirroring lexical scopes.
- `StrVec` stores deduplicated string literals for emission into the data section.
- `Codegen` holds the output file handle along with state such as `next_label`, current `scope`, and stack tracking (`frame_size`, `stack_depth`).

## Key Functions
- `codegen_create`/`codegen_free` allocate and dispose of a `Codegen` instance.
- `codegen_program` walks the AST and emits assembly for the `main` function.
- Internal helpers like `gen_expr` and `emit_node` handle specific node kinds, while `scope_push`/`scope_pop` manage symbol scopes.

## Example Workflow
```c
Codegen *cg = codegen_create(stdout);
codegen_program(cg, program);
codegen_free(cg);
```

## Extending
To emit code for a new AST node:
1. Extend `gen_expr` or `emit_node` with a case for the new `NodeKind`.
2. Manage any new runtime calls or helper routines (e.g. add them to the runtime and emit the appropriate `call`).
3. If new data types are involved, ensure symbols track any additional metadata needed by later stages.
