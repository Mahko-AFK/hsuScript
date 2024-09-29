#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"

int main(int argc, char *argv[]) {
  FILE *file;
  file = fopen(argv[1], "r");

  if(!file){
    printf("ERROR: File not found\n");
    exit(1);
  }

  Token *tokens = lexer(file);
  
  int counter = 0;
  while(tokens[counter].type != END_OF_TOKENS){
    print_token(tokens[counter]);
    counter++;
  }

  Node *root = parser(tokens);
  
  printf("Printing AST (Abstract Syntax Tree):\n");
  print_tree(root, 0, "Root", 0);
}
