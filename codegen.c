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

/* Simple scope stack ------------------------------------------------------ */
typedef struct Scope {
    Symbol *items;
    size_t len;
    size_t cap;
    struct Scope *parent;
} Scope;

typedef struct {
    char **items;
    size_t len;
    size_t cap;
} StrVec;

struct Codegen {
    FILE *out;
    int next_label;
    StrVec strs;
    Scope *scope;         /* current innermost scope */
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

static int count_locals(Node *node) {
    if (!node) return 0;
    if (node->kind == NK_FnDecl) return 0;
    int n = (node->kind == NK_LetStmt) ? 1 : 0;
    n += count_locals(node->left);
    n += count_locals(node->right);
    for (size_t i = 0; i < node->children.len; i++)
        n += count_locals(node->children.items[i]);
    return n;
}

/* ------------------------------------------------------------------------- */
/* Scope and symbol helpers                                                 */

static void scope_push(Codegen *cg) {
    Scope *s = calloc(1, sizeof(Scope));
    if (!s) return;
    s->items = NULL;
    s->len = s->cap = 0;
    s->parent = cg->scope;
    cg->scope = s;
}

static void scope_pop(Codegen *cg) {
    if (!cg->scope) return;
    Scope *s = cg->scope;
    cg->scope = s->parent;
    for (size_t i = 0; i < s->len; i++)
        free((char *)s->items[i].name);
    free(s->items);
    free(s);
}

static int sym_add(Codegen *cg, const char *name, bool is_string) {
    if (!cg->scope) return -1;
    Scope *s = cg->scope;
    if (s->len == s->cap) {
        s->cap = s->cap ? s->cap * 2 : 8;
        s->items = realloc(s->items, s->cap * sizeof(*s->items));
    }
    cg->frame_size += 8;
    s->items[s->len].name = strdup(name);
    s->items[s->len].offset = cg->frame_size;
    s->items[s->len].is_string = is_string;
    s->len++;
    return cg->frame_size;
}

static int sym_lookup(Codegen *cg, const char *name, bool *is_string) {
    for (Scope *s = cg->scope; s; s = s->parent) {
        for (size_t i = 0; i < s->len; i++) {
            if (strcmp(s->items[i].name, name) == 0) {
                if (is_string) *is_string = s->items[i].is_string;
                return s->items[i].offset;
            }
        }
    }
    return -1;
}

/* ------------------------------------------------------------------------- */
/* Expression and statement emission                                        */

static void emit_expr(Codegen *cg, Node *node);

Codegen *codegen_create(FILE *out) {
    Codegen *cg = calloc(1, sizeof(Codegen));
    if (!cg) return NULL;
    cg->out = out;
    cg->next_label = 0;
    cg->strs.items = NULL;
    cg->strs.len = cg->strs.cap = 0;
    cg->scope = NULL;
    cg->frame_size = 0;
    return cg;
}

void codegen_free(Codegen *cg) {
    if (!cg) return;
    for (size_t i = 0; i < cg->strs.len; i++) {
        free(cg->strs.items[i]);
    }
    free(cg->strs.items);
    /* free scope chain */
    for (Scope *s = cg->scope; s;) {
        Scope *parent = s->parent;
        for (size_t i = 0; i < s->len; i++)
            free((char*)s->items[i].name);
        free(s->items);
        free(s);
        s = parent;
    }
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

static void emit_expr(Codegen *cg, Node *node) {
    if (!node) return;
    switch (node->kind) {
    case NK_Int:
        emit(cg, "    mov rax, %s\n", node->value ? node->value : "0");
        break;
    case NK_String: {
        size_t idx = intern_str(cg, node->value ? node->value : "");
        emit(cg, "    lea rax, [rel str%zu]\n", idx);
        break;
    }
    case NK_Identifier: {
        int off = sym_lookup(cg, node->value, NULL);
        if (off >= 0)
            emit(cg, "    mov rax, [rbp - %d]\n", off);
        break;
    }
    case NK_Assign: {
        emit_expr(cg, node->right);
        int off = sym_lookup(cg, node->left->value, NULL);
        if (off >= 0)
            emit(cg, "    mov [rbp - %d], rax\n", off);
        break;
    }
    case NK_Binary: {
        if (node->op == PLUS || node->op == DASH || node->op == STAR) {
            emit_expr(cg, node->left);
            emit(cg, "    push rax\n");
            emit_expr(cg, node->right);
            emit(cg, "    pop rbx\n");
            switch (node->op) {
            case PLUS:
                emit(cg, "    add rax, rbx\n");
                break;
            case DASH:
                emit(cg, "    sub rbx, rax\n    mov rax, rbx\n");
                break;
            case STAR:
                emit(cg, "    imul rax, rbx\n");
                break;
            default:
                break;
            }
        }
        break;
    }
    default:
        break;
    }
}

static void emit_node(Codegen *cg, Node *node, bool *has_exit) {
    if (!node) return;
    switch (node->kind) {
    case NK_Program:
        scope_push(cg);
        for (size_t i = 0; i < node->children.len; i++)
            emit_node(cg, node->children.items[i], has_exit);
        scope_pop(cg);
        break;
    case NK_FnDecl: {
        const char *name = node->value ? node->value : "fn";
        Node *body = node->children.len > 0 ? node->children.items[0] : NULL;
        int locals = count_locals(body);
        int frame = locals * 8;
        if ((frame + 8) % 16 != 0)
            frame += 8;
        emit(cg, "%s:\n", name);
        emit(cg, "    push rbp\n");
        emit(cg, "    mov rbp, rsp\n");
        emit(cg, "    sub rsp, %d\n", frame);
        int saved = cg->frame_size;
        cg->frame_size = 0;
        if (body)
            emit_node(cg, body, has_exit);
        cg->frame_size = saved;
        emit(cg, "    mov rsp, rbp\n");
        emit(cg, "    pop rbp\n");
        emit(cg, "    xor eax, eax\n");
        emit(cg, "    ret\n");
        break;
    }
    case NK_Block:
        scope_push(cg);
        for (size_t i = 0; i < node->children.len; i++)
            emit_node(cg, node->children.items[i], has_exit);
        scope_pop(cg);
        break;
    case NK_LetStmt: {
        bool is_str = node->right && node->right->kind == NK_String;
        sym_add(cg, node->value, is_str);
        if (node->right) {
            emit_expr(cg, node->right);
            int off = sym_lookup(cg, node->value, NULL);
            if (off >= 0)
                emit(cg, "    mov [rbp - %d], rax\n", off);
        }
        break;
    }
    case NK_AssignStmt: {
        emit_expr(cg, node->right);
        int off = sym_lookup(cg, node->value, NULL);
        if (off >= 0)
            emit(cg, "    mov [rbp - %d], rax\n", off);
        break;
    }
    case NK_ExprStmt:
        emit_expr(cg, node->left);
        break;
    case NK_ExitStmt:
        emit_exit(cg, node, has_exit);
        break;
    default:
        /* generic traversal */
        emit_node(cg, node->left, has_exit);
        emit_node(cg, node->right, has_exit);
        for (size_t i = 0; i < node->children.len; i++)
            emit_node(cg, node->children.items[i], has_exit);
        break;
    }
}

void codegen_program(Codegen *cg, Node *program) {
    if (!cg || !cg->out) return;
    bool has_exit = false;
    emit(cg, "section .text\n");
    emit(cg, "global _start\n");
    emit(cg, "_start:\n");
    emit(cg, "    and rsp, -16\n");
    emit(cg, "    call main\n");
    emit(cg, "    mov rax, 60\n");
    emit(cg, "    xor rdi, rdi\n");
    emit(cg, "    syscall\n");
    emit_node(cg, program, &has_exit);
    if (cg->strs.len > 0) {
        emit(cg, "section .data\n");
        for (size_t i = 0; i < cg->strs.len; i++) {
            const char *s = cg->strs.items[i];
            emit(cg, "str%zu: db \"", i);
            for (const char *p = s; *p; p++) {
                if (*p == '"' || *p == '\\')
                    emit(cg, "\\%c", *p);
                else if (*p == '\n')
                    emit(cg, "\",10,\"");
                else
                    emit(cg, "%c", *p);
            }
            emit(cg, "\",0\n");
        }
    }
}

