#!/bin/bash
mkdir -p build
gcc -Iinclude main.c lexer.c parser.c tools.c sem.c codegen.c -o build/hsc -Wall -Wextra
