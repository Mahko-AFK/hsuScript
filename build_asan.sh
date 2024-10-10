#!/bin/bash
mkdir -p build
gcc -c main.c lexer.c parser.c tools.c -fsanitize=address -g || { echo "Compilation failed"; exit 1; }
gcc main.o lexer.o parser.o tools.o -o build/hsu_asan -fsanitize=address -static-libasan || { echo "Linking failed"; exit 1; }
rm -f main.o lexer.o parser.o tools.o
echo "Build completed successfully!"

