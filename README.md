# hsuScript
hsuScript is a small educational compiler written in C that demonstrates the complete pipeline from lexical analysis to runtime execution, aiming to provide an approachable example for developers interested in learning how compilers work and how languages are designed.

## About hsuScript

hsuScript breaks source files into tokens, builds an abstract syntax tree, generates code, and executes the result through a minimal runtime.

```mermaid
flowchart LR
    A[Source] --> B[Lexer]
    B --> C[Parser]
    C --> D[Codegen]
    D --> E[Runtime]
```

## Features

- Written in portable C
- Lexer and parser producing an AST
- Simple code generation and runtime support
- Scripts with variables, control flow, strings, and arithmetic
- Comprehensive test suite

## Project Structure

```text
.
├── docs/          # module documentation
├── lexer.c        # tokenizes source code
├── parser.c       # builds the AST
├── sem.c          # semantic analysis
├── codegen.c      # emits code
├── runtime/       # runtime support library
├── tests/         # parser and execution tests
└── tools/         # build and test scripts
```

Module guides:
- [Lexer](docs/lexer.md)
- [Parser](docs/parser.md)
- [Semantics](docs/semantics.md)
- [Code generation](docs/codegen.md)
- [Runtime](docs/runtime.md)

## Quick Start

1. Build the compiler:
   ```bash
   ./tools/build.sh
   ```
2. Compile and run a script:
   ```bash
   ./build/hsc path/to/file.hsc
   ```

## Command-line options

- `--ast-only`: parse and print the AST without generating code
- `--emit-asm [path]`: write assembly to `path` (defaults to `build/out.s`)
- `--compile [output]`: produce a binary named `output` (defaults to `a.out`) without running it

## Testing

Run parser fixtures:

```bash
./tools/runtests.sh
```

Run the full test suite, including execution checks:

```bash
./tools/run_all_tests.sh
```

## Contributing

- Fork the repository and create a feature branch
- Make your changes and run the test suite
- Submit a pull request describing your work

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

