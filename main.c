#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>
#include <stddef.h>

#include "lexer.h"
#include "parser.h"
#include "tools.h"
#include "codegen.h"
#include "sem.h"

extern unsigned char rt_o_start[];
extern unsigned char rt_o_end[];

static int dump_runtime(const char *path) {
  FILE *f = fopen(path, "wb");
  if (!f) return -1;
  size_t size = (size_t)(rt_o_end - rt_o_start);
  if (fwrite(rt_o_start, 1, size, f) != size) {
    fclose(f);
    return -1;
  }
  fclose(f);
  return 0;
}

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
  int compile_bin = 0;
  const char *bin_path = NULL;
  int run_bin = 0;
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
    } else if (strcmp(argv[argi], "--compile") == 0) {
      compile_bin = 1;
      bin_path = "a.out";
      if (argc > argi + 1 && argv[argi + 1][0] != '-') {
        bin_path = argv[argi + 1];
        argi += 2;
      } else {
        argi++;
      }
    } else if (strcmp(argv[argi], "--dump-rt") == 0) {
      if (argc <= argi + 1) {
        fprintf(stderr, "--dump-rt requires a path\n");
        return 1;
      }
      if (dump_runtime(argv[argi + 1]) != 0) {
        fprintf(stderr, "ERROR: could not write runtime object\n");
        return 1;
      }
      return 0;
    } else {
      break;
    }
  }

  if (argc <= argi) {
    fprintf(stderr, "Usage: %s [--ast-only] [--emit-asm [path]] [--compile [output]] <file>\n", argv[0]);
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

  if (!compile_bin && emit_path == NULL) {
    emit_path = "build/out.s";
    bin_path = "build/out";
    compile_bin = 1;
    run_bin = 1;
  } else {
    if (emit_path == NULL) {
      emit_path = "build/out.s";
    }
    if (compile_bin && bin_path == NULL) {
      bin_path = "a.out";
    }
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
  if (dump_runtime("build/rt_tmp.o") != 0) {
    fprintf(stderr, "ERROR: failed to write runtime object\n");
    free_tree(root);
    return 1;
  }
  if (compile_bin) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "gcc -Wa,--noexecstack -c %s -o build/out.o", emit_path);
    if (system(cmd) != 0) {
      fprintf(stderr, "ERROR: failed to assemble output\n");
      free_tree(root);
      return 1;
    }
    snprintf(cmd, sizeof(cmd), "gcc build/out.o build/rt_tmp.o -o %s", bin_path);
    if (system(cmd) != 0) {
      fprintf(stderr, "ERROR: failed to link binary\n");
      free_tree(root);
      return 1;
    }
  }

  if (run_bin) {
    char cmd[512];
    if (bin_path[0] == '/' || strncmp(bin_path, "./", 2) == 0) {
      snprintf(cmd, sizeof(cmd), "%s", bin_path);
    } else {
      snprintf(cmd, sizeof(cmd), "./%s", bin_path);
    }
    int rc = system(cmd);
    if (rc != -1) {
      rc = WEXITSTATUS(rc);
    }
    free_tree(root);
    return rc;
  }

  free_tree(root);
  return 0;
}
