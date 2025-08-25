# hsuScript

hsuScript is a toy compiler written in C.

## Build

Compile the project with:

```bash
./tools/build.sh
```

The executable is placed at `build/hsc`. AddressSanitizer build is available via `./tools/build_asan.sh`.

## Running

Run the compiler on a `.hsc` source file to print tokens and the AST:

```bash
./build/hsc path/to/file.hsc
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

## Testing

Run the parser fixtures with:

```bash
./tools/runtests.sh
```

Run the full test suite, including executing compiled programs and
verifying their stdout, with:

```bash
./tools/run_all_tests.sh
```

These scripts build the compiler, check each `.hsc` in `tests/cases`
against its expected output, and report results to stdout.
