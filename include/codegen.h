#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "parser.h"

typedef struct Codegen Codegen;

Codegen *codegen_create(FILE *out);
void codegen_free(Codegen *cg);
void codegen_program(Codegen *cg, Node *program);

#endif
