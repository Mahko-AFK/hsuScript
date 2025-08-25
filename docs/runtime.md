# Runtime

The runtime library supplies helper functions that the generated assembly calls for I/O and string handling.

## Functions
- `hsu_print_cstr(const char *s)` prints a C string followed by a newline.
- `hsu_print_int(long n)` prints an integer value.
- `hsu_concat(const char *a, const char *b)` allocates and returns the concatenation of two strings.

## Example Workflow
These functions are compiled into a static library and linked with the code produced by `codegen`. For example, a `write("hi")` statement becomes a call to `hsu_print_cstr`.

## Extending
To add a new runtime capability:
1. Implement the function in `runtime/rt.c` and declare it in `include/runtime.h`.
2. Rebuild the runtime library via `tools/build.sh`.
3. Update the code generator so that it emits calls to the new function when appropriate.
