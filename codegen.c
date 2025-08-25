#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "codegen.h"

typedef struct {
    const char *name;
    int offset;
    bool is_string;
} Symbol;

typedef struct {
    Symbol *items;
    size_t len;
    size_t cap;
} SymbolVec;

typedef struct {
    char **items;
    size_t len;
    size_t cap;
} StrVec;

struct Codegen {
    FILE *out;
    int next_label;
    StrVec strs;
    SymbolVec scope;
    int frame_size;
};

static void emit(Codegen *cg, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(cg->out, fmt, ap);
    va_end(ap);
}

static int new_label(Codegen *cg) __attribute__((unused));
static int new_label(Codegen *cg) {
    return cg->next_label++;
}

static size_t intern_str(Codegen *cg, const char *s) __attribute__((unused));
static size_t intern_str(Codegen *cg, const char *s) {
    for (size_t i = 0; i < cg->strs.len; i++) {
        if (strcmp(cg->strs.items[i], s) == 0) {
            return i;
        }
    }
    if (cg->strs.len == cg->strs.cap) {
        cg->strs.cap = cg->strs.cap ? cg->strs.cap * 2 : 8;
        cg->strs.items = realloc(cg->strs.items, cg->strs.cap * sizeof(*cg->strs.items));
    }
    cg->strs.items[cg->strs.len] = strdup(s);
    return cg->strs.len++;
}

Codegen *codegen_create(FILE *out) {
    Codegen *cg = calloc(1, sizeof(Codegen));
    if (!cg) return NULL;
    cg->out = out;
    cg->next_label = 0;
    cg->strs.items = NULL;
    cg->strs.len = cg->strs.cap = 0;
    cg->scope.items = NULL;
    cg->scope.len = cg->scope.cap = 0;
    cg->frame_size = 0;
    return cg;
}

void codegen_free(Codegen *cg) {
    if (!cg) return;
    for (size_t i = 0; i < cg->strs.len; i++) {
        free(cg->strs.items[i]);
    }
    free(cg->strs.items);
    free(cg->scope.items);
    free(cg);
}

static void emit_exit(Codegen *cg, Node *node, bool *has_exit) {
    Node *paren = node->left;
    Node *arg = paren ? paren->left : NULL;
    const char *status = arg && arg->value ? arg->value : "0";
    emit(cg, "    mov rax, 60\n");
    emit(cg, "    mov rdi, %s\n", status);
    emit(cg, "    syscall\n");
    *has_exit = true;
}

static void emit_node(Codegen *cg, Node *node, bool *has_exit) {
    if (!node) return;
    if (node->kind == NK_ExitStmt) {
        emit_exit(cg, node, has_exit);
    }
    emit_node(cg, node->left, has_exit);
    emit_node(cg, node->right, has_exit);
    for (size_t i = 0; i < node->children.len; i++) {
        emit_node(cg, node->children.items[i], has_exit);
    }
}

void codegen_program(Codegen *cg, Node *program) {
    if (!cg || !cg->out) return;
    bool has_exit = false;
    emit(cg, "section .text\n");
    emit(cg, "global _start\n");
    emit(cg, "_start:\n");
    emit_node(cg, program, &has_exit);
    if (!has_exit) {
        emit(cg, "    mov rax, 60\n");
        emit(cg, "    mov rdi, 0\n");
        emit(cg, "    syscall\n");
    }
}

