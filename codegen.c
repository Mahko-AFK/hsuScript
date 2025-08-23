#include <stdio.h>
#include "parser.h"
#include "codegen.h"

static int has_exit = 0;

static void emit_exit(Node *node, FILE *out) {
    Node *paren = node->left;
    Node *arg = paren ? paren->left : NULL;
    const char *status = arg && arg->value ? arg->value : "0";
    fprintf(out, "    mov rax, 60\n");
    fprintf(out, "    mov rdi, %s\n", status);
    fprintf(out, "    syscall\n");
    has_exit = 1;
}

static void emit_node(Node *node, FILE *out) {
    if (!node) return;
    if (node->type == EXIT) {
        emit_exit(node, out);
    }
    emit_node(node->left, out);
    emit_node(node->right, out);
}

void generate_code(Node *root, const char *filename) {
    FILE *out = fopen(filename, "w");
    if (!out) {
        perror("codegen");
        return;
    }
    has_exit = 0;
    fprintf(out, "section .text\n");
    fprintf(out, "global _start\n");
    fprintf(out, "_start:\n");
    emit_node(root, out);
    if (!has_exit) {
        fprintf(out, "    mov rax, 60\n");
        fprintf(out, "    mov rdi, 0\n");
        fprintf(out, "    syscall\n");
    }
    fclose(out);
}
