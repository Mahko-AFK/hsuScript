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
  int ast_only = 0;
  int argi = 1;

  if (argc > argi && strcmp(argv[argi], "--ast-only") == 0) {
    ast_only = 1;
    argi++;
  }

  if (argc <= argi) {
    fprintf(stderr, "Usage: %s [--ast-only] <file>\n", argv[0]);
    return 1;
  }

  FILE *file = fopen(argv[argi], "r");

  if(!file){
    printf("ERROR: File not found\n");
    exit(1);
  }

  Token *tokens = lexer(file);

  if(!ast_only){
    print_tokens(tokens);
  }

  Node *root = parser(tokens);

  printf("Printing AST (Abstract Syntax Tree):\n");
  print_tree(root, 0);

  fflush(stdout);

  sem_program(root);

  generate_code(root, "out.asm");
  free_tree(root);

}
