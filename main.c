#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>

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
  const char *emit_path = NULL;
  int argi = 1;

  while (argc > argi) {
    if (strcmp(argv[argi], "--ast-only") == 0) {
      ast_only = 1;
      argi++;
    } else if (strcmp(argv[argi], "--emit-asm") == 0) {
      emit_path = "build/out.s";
      if (argc > argi + 2 && argv[argi + 1][0] != '-') {
        emit_path = argv[argi + 1];
        argi += 2;
      } else {
        argi++;
      }
    } else {
      break;
    }
  }

  if (argc <= argi) {
    fprintf(stderr, "Usage: %s [--ast-only] [--emit-asm [path]] <file>\n", argv[0]);
    return 1;
  }

  FILE *file = fopen(argv[argi], "r");

  if(!file){
    printf("ERROR: File not found\n");
    exit(1);
  }

  Token *tokens = lexer(file);

  if (ast_only) {
    Node *root = parser(tokens);
    printf("Printing AST (Abstract Syntax Tree):\n");
    print_tree(root, 0);
    fflush(stdout);
    free_tree(root);
    return 0;
  }

  Node *root = parser(tokens);
  sem_program(root);

  int run_bin = 0;
  if (emit_path == NULL) {
    emit_path = "build/out.s";
    run_bin = 1;
  }

  if (system("mkdir -p build") != 0) {
    fprintf(stderr, "ERROR: could not create build directory\n");
    free_tree(root);
    return 1;
  }

  FILE *outf = fopen(emit_path, "w");
  if (!outf) {
    fprintf(stderr, "ERROR: Could not open %s for writing\n", emit_path);
    free_tree(root);
    return 1;
  }

  Codegen *cg = codegen_create(outf);
  codegen_program(cg, root);
  codegen_free(cg);
  fclose(outf);

  if (run_bin) {
    if (system("gcc -Wa,--noexecstack -c build/out.s -o build/out.o") != 0 ||
        system("gcc build/out.o build/rt.o -o build/out") != 0) {
      fprintf(stderr, "ERROR: failed to build binary\n");
      free_tree(root);
      return 1;
    }
    int rc = system("./build/out");
    if (rc != -1) {
      rc = WEXITSTATUS(rc);
    }
    free_tree(root);
    return rc;
  }

  free_tree(root);
  return 0;
}
