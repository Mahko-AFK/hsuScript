#!/bin/bash
mkdir -p build
gcc -Iinclude -c main.c lexer.c parser.c tools.c sem.c codegen.c -fsanitize=address -g || { echo "Compilation failed"; exit 1; }
gcc -Iinclude main.o lexer.o parser.o tools.o sem.o codegen.o -o build/hsu_asan -fsanitize=address -static-libasan || { echo "Linking failed"; exit 1; }
rm -f main.o lexer.o parser.o tools.o sem.o codegen.o
echo "Build completed successfully!"
