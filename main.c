#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "lexer.h"
#include "parser.h"
#include "tools.h"
#include "codegen.h"
#include "sem.h"

void print_tokens(Token *t) {
  size_t i = 0;
  while(t[i].value != NULL){
    print_token(t[i]);
    i++;
  }
}

int main(int argc, char *argv[]) {
  FILE *file;
  file = fopen(argv[1], "r");

  if(!file){
    printf("ERROR: File not found\n");
    exit(1);
  }

  Token *tokens = lexer(file);
  
  print_tokens(tokens);
  
  Node *root = parser(tokens);

  printf("Printing AST (Abstract Syntax Tree):\n");
  print_tree(root, 0, "Root", 0);

  sem_program(root);

  generate_code(root, "out.asm");
  free_tree(root);

}
