# hsuScript

hsuScript is a toy compiler written in C.

## Build

Compile the project with:

```bash
./build.sh
```

The executable is placed at `build/hsc`. AddressSanitizer build is available via `./build_asan.sh`.

## Running

Run the compiler on a `.hs` source file to print tokens and the AST:

```bash
./build/hsc path/to/file.hs
```

## Language Syntax

- Entry point defined as `fn main() { ... }`
- Variables declared with `let name = value;`
- Strings use double quotes and can be concatenated with `+`
- Arithmetic operators: `+`, `-`, `*`, `/`, `%`
- Conditionals: `if`, `elif`, `else`
- Loops: `for(init; condition; update) { ... }`
- Output using `write(expression);`

See `test_cases/` for examples.
