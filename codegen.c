#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "codegen.h"
#include "sem.h"

typedef struct {
    const char *name;
    int offset;
    bool is_string;
} Symbol;

/* Simple scope stack ------------------------------------------------------ */
typedef struct CGScope {
    Symbol *items;
    size_t len;
    size_t cap;
    struct CGScope *parent;
} CGScope;

typedef struct {
    char **items;
    size_t len;
    size_t cap;
} StrVec;

struct Codegen {
    FILE *out;
    int next_label;
    StrVec strs;
    CGScope *scope;         /* current innermost scope */
    int frame_size;         /* bytes of locals allocated so far */
    int stack_depth;        /* bytes pushed on stack since prologue */
};

static void emit(Codegen *cg, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(cg->out, fmt, ap);
    va_end(ap);
}

/* System V AMD64 ABI requires %rsp+8 to be 16-byte aligned at call sites.
   frame_size tracks our stack frame for locals and stack_depth counts
   any pushes since the function prologue. */
static void emit_call(Codegen *cg, const char *target) {
    int total = cg->frame_size + cg->stack_depth + 8;
    int fix = 0;
    if (total % 16 != 0) {
        assert(total % 16 == 8 && "stack misaligned by non-8 bytes");
        emit(cg, "    sub rsp, 8\n");
        fix = 8;
        total += 8;
    }
    assert(total % 16 == 0 && "stack misaligned before call");
    emit(cg, "    call %s\n", target);
    if (fix)
        emit(cg, "    add rsp, 8\n");
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
    CGScope *s = calloc(1, sizeof(CGScope));
    if (!s) return;
    s->items = NULL;
    s->len = s->cap = 0;
    s->parent = cg->scope;
    cg->scope = s;
}

static void scope_pop(Codegen *cg) {
    if (!cg->scope) return;
    CGScope *s = cg->scope;
    cg->scope = s->parent;
    for (size_t i = 0; i < s->len; i++)
        free((char *)s->items[i].name);
    free(s->items);
    free(s);
}

static int sym_add(Codegen *cg, const char *name, bool is_string) {
    if (!cg->scope) return -1;
    CGScope *s = cg->scope;
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
    for (CGScope *s = cg->scope; s; s = s->parent) {
        for (size_t i = 0; i < s->len; i++) {
            if (strcmp(s->items[i].name, name) == 0) {
                if (is_string) *is_string = s->items[i].is_string;
                return s->items[i].offset;
            }
        }
    }
    return -1;
}

static void sym_set_is_string(Codegen *cg, const char *name, bool is_string) {
    for (CGScope *s = cg->scope; s; s = s->parent) {
        for (size_t i = 0; i < s->len; i++) {
            if (strcmp(s->items[i].name, name) == 0) {
                s->items[i].is_string = is_string;
                return;
            }
        }
    }
}

static void sym_set_string(Codegen *cg, int offset, bool is_string) {
    for (CGScope *s = cg->scope; s; s = s->parent) {
        for (size_t i = 0; i < s->len; i++) {
            if (s->items[i].offset == offset) {
                s->items[i].is_string = is_string;
                return;
            }
        }
    }
}

/* ------------------------------------------------------------------------- */
/* Expression and statement emission                                        */

static void gen_expr(Codegen *cg, Node *node);

Codegen *codegen_create(FILE *out) {
    Codegen *cg = calloc(1, sizeof(Codegen));
    if (!cg) return NULL;
    cg->out = out;
    cg->next_label = 0;
    cg->strs.items = NULL;
    cg->strs.len = cg->strs.cap = 0;
    cg->scope = NULL;
    cg->frame_size = 0;
    cg->stack_depth = 0;
    return cg;
}

void codegen_free(Codegen *cg) {
    if (!cg) return;
    for (size_t i = 0; i < cg->strs.len; i++) {
        free(cg->strs.items[i]);
    }
    free(cg->strs.items);
    /* free scope chain */
    for (CGScope *s = cg->scope; s;) {
        CGScope *parent = s->parent;
        for (size_t i = 0; i < s->len; i++)
            free((char*)s->items[i].name);
        free(s->items);
        free(s);
        s = parent;
    }
    free(cg);
}

static void emit_exit(Codegen *cg, Node *node, bool *has_exit) {
    if (node && node->left) {
        gen_expr(cg, node->left);
        emit(cg, "    mov edi, eax\n");
    } else {
        emit(cg, "    xor edi, edi\n");
    }
    emit_call(cg, "exit@PLT");
    *has_exit = true;
}

static void gen_expr(Codegen *cg, Node *node) {
    if (!node) return;

    switch (node->kind) {
    case NK_Int:
        emit(cg, "    mov rax, %s\n", node->value ? node->value : "0");
        break;
    case NK_Bool:
        emit(cg, "    mov rax, %s\n", (node->value && strcmp(node->value, "true") == 0) ? "1" : "0");
        break;
    case NK_String: {
        size_t idx = intern_str(cg, node->value ? node->value : "");
        emit(cg, "    lea rax, [rip + .Lstr%zu]\n", idx);
        break;
    }
    case NK_Identifier: {
        int off = sym_lookup(cg, node->value, NULL);
        if (off >= 0) {
            emit(cg, "    mov rax, [rbp - %d]\n", off);
        } else {
            fprintf(stderr, "codegen: unknown symbol %s\n",
                    node->value ? node->value : "<null>");
            exit(1);
        }
        break;
    }
    case NK_Unary:
        gen_expr(cg, node->left);
        switch (node->op) {
        case DASH:
            emit(cg, "    neg rax\n");
            break;
        case NOT:
            emit(cg, "    test rax, rax\n    sete al\n    movzx rax, al\n");
            break;
        case PLUS_PLUS:
        case MINUS_MINUS: {
            if (node->left && node->left->kind == NK_Identifier) {
                int off = sym_lookup(cg, node->left->value, NULL);
                if (off >= 0) {
                    if (node->postfix)
                        emit(cg, "    mov rcx, rax\n");
                    if (node->op == PLUS_PLUS)
                        emit(cg, "    add rax, 1\n");
                    else
                        emit(cg, "    sub rax, 1\n");
                    emit(cg, "    mov [rbp - %d], rax\n", off);
                    if (node->postfix)
                        emit(cg, "    mov rax, rcx\n");
                }
            }
            break;
        }
        default:
            break;
        }
        break;
    case NK_Assign:
        gen_expr(cg, node->right);
        if (node->left && node->left->kind == NK_Identifier) {
            const char *name = node->left->value;
            int off = sym_lookup(cg, name, NULL);
            if (off >= 0) {
                emit(cg, "    mov [rbp - %d], rax\n", off);
                bool is_str = node->right &&
                              (node->right->kind == NK_String ||
                               (node->right->kind == NK_Binary &&
                                node->right->op == PLUS &&
                                node->right->ty && node->right->ty->kind == TY_STRING));
                sym_set_string(cg, off, is_str);
            } else {
                fprintf(stderr, "codegen: unknown symbol %s\n",
                        name ? name : "<null>");
                exit(1);
            }
        }
        break;
    case NK_Binary:
        if (node->op == AND || node->op == OR) {
            int l_short = new_label(cg);
            int l_end = new_label(cg);
            if (node->op == AND) {
                gen_expr(cg, node->left);
                emit(cg, "    cmp rax, 0\n    je .L%d\n", l_short);
                gen_expr(cg, node->right);
                emit(cg, "    cmp rax, 0\n    setne al\n    movzx rax, al\n    jmp .L%d\n", l_end);
                emit(cg, ".L%d:\n    xor eax, eax\n", l_short);
                emit(cg, ".L%d:\n", l_end);
            } else { /* OR */
                gen_expr(cg, node->left);
                emit(cg, "    cmp rax, 0\n    jne .L%d\n", l_short);
                gen_expr(cg, node->right);
                emit(cg, "    cmp rax, 0\n    setne al\n    movzx rax, al\n    jmp .L%d\n", l_end);
                emit(cg, ".L%d:\n    mov eax, 1\n", l_short);
                emit(cg, ".L%d:\n", l_end);
            }
            break;
        }

        bool left_is_str = node->left &&
                           ((node->left->ty && node->left->ty->kind == TY_STRING) ||
                            node->left->kind == NK_String);
        if (!left_is_str && node->left && node->left->kind == NK_Identifier)
            sym_lookup(cg, node->left->value, &left_is_str);
        bool right_is_str = node->right &&
                             ((node->right->ty && node->right->ty->kind == TY_STRING) ||
                              node->right->kind == NK_String);
        if (!right_is_str && node->right && node->right->kind == NK_Identifier)
            sym_lookup(cg, node->right->value, &right_is_str);
        if (node->op == PLUS && (left_is_str || right_is_str)) {
            gen_expr(cg, node->left);
            emit(cg, "    push rax\n");
            cg->stack_depth += 8;
            gen_expr(cg, node->right);
            emit(cg, "    mov rsi, rax\n    pop rdi\n");
            cg->stack_depth -= 8;
            emit_call(cg, "hsu_concat@PLT");
            node->ty = type_string();
            break;
        }

        gen_expr(cg, node->left);
        emit(cg, "    push rax\n");
        cg->stack_depth += 8;
        gen_expr(cg, node->right);
        emit(cg, "    mov r10, rax\n    pop rax\n");
        cg->stack_depth -= 8;

        switch (node->op) {
        case PLUS:
            emit(cg, "    add rax, r10\n");
            break;
        case DASH:
            emit(cg, "    sub rax, r10\n");
            break;
        case STAR:
            emit(cg, "    imul rax, r10\n");
            break;
        case SLASH:
            emit(cg, "    cqo\n    idiv r10\n");
            break;
        case PERCENT:
            emit(cg, "    cqo\n    idiv r10\n    mov rax, rdx\n");
            break;
        case EQUALS:
            emit(cg, "    cmp rax, r10\n    sete al\n    movzx rax, al\n");
            break;
        case NOT_EQUALS:
            emit(cg, "    cmp rax, r10\n    setne al\n    movzx rax, al\n");
            break;
        case LESS:
            emit(cg, "    cmp rax, r10\n    setl al\n    movzx rax, al\n");
            break;
        case LESS_EQUALS:
            emit(cg, "    cmp rax, r10\n    setle al\n    movzx rax, al\n");
            break;
        case GREATER:
            emit(cg, "    cmp rax, r10\n    setg al\n    movzx rax, al\n");
            break;
        case GREATER_EQUALS:
            emit(cg, "    cmp rax, r10\n    setge al\n    movzx rax, al\n");
            break;
        default:
            break;
        }
        break;
    default:
        fprintf(stderr, "codegen: unsupported node kind %d\n", node->kind);
        exit(1);
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
        const char *name = node->value ? node->value : "<null>";
        Node *body = node->children.len > 0 ? node->children.items[0] : NULL;
        int locals = count_locals(body);
        int frame = locals * 8;
        if ((frame + 8) % 16 != 0)
            frame += 8;
        emit(cg, "%s:\n", name);
        emit(cg, "    push rbp\n");
        emit(cg, "    mov rbp, rsp\n");
        emit(cg, "    sub rsp, %d\n", frame);
        int saved_fs = cg->frame_size;
        int saved_sd = cg->stack_depth;
        cg->frame_size = 0;
        cg->stack_depth = 0;
        if (body)
            emit_node(cg, body, has_exit);
        cg->frame_size = saved_fs;
        cg->stack_depth = saved_sd;
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
        sym_add(cg, node->value, false);
        if (node->right) {
            gen_expr(cg, node->right);
            int off = sym_lookup(cg, node->value, NULL);
            if (off >= 0)
                emit(cg, "    mov [rbp - %d], rax\n", off);
            bool is_str = node->right &&
                          (node->right->kind == NK_String ||
                           (node->right->ty && node->right->ty->kind == TY_STRING));
            sym_set_is_string(cg, node->value, is_str);
        }
        break;
    }
    case NK_AssignStmt: {
        gen_expr(cg, node->right);
        const char *name = (node->left && node->left->value) ? node->left->value : NULL;
        int off = sym_lookup(cg, name, NULL);
        if (off >= 0) {
            emit(cg, "    mov [rbp - %d], rax\n", off);
            bool is_str = node->right &&
                          (node->right->kind == NK_String ||
                           (node->right->kind == NK_Binary &&
                            node->right->op == PLUS &&
                            node->right->ty && node->right->ty->kind == TY_STRING));
            sym_set_string(cg, off, is_str);
        } else {
            fprintf(stderr, "codegen: unknown symbol %s\n",
                    name ? name : "<null>");
            exit(1);
        }
        break;
    }
    case NK_WriteStmt: {
        bool is_str = false;
        if (node->left) {
            if (node->left->kind == NK_String)
                is_str = true;
            else if (node->left->kind == NK_Identifier)
                sym_lookup(cg, node->left->value, &is_str);
            else if (node->left->ty && node->left->ty->kind == TY_STRING)
                is_str = true;
        }
        gen_expr(cg, node->left);
        emit(cg, "    mov rdi, rax\n");
        if (is_str)
            emit_call(cg, "hsu_print_cstr@PLT");
        else
            emit_call(cg, "hsu_print_int@PLT");
        break;
    }
    case NK_ExprStmt:
        gen_expr(cg, node->left);
        break;
    case NK_ExitStmt:
        emit_exit(cg, node, has_exit);
        break;
    case NK_IfStmt: {
        Node *cond = node->children.len > 0 ? node->children.items[0] : NULL;
        Node *then_block = node->children.len > 1 ? node->children.items[1] : NULL;
        Node *else_node = node->children.len > 2 ? node->children.items[2] : NULL;
        int l_else = new_label(cg);
        int l_end = new_label(cg);
        if (cond)
            gen_expr(cg, cond);
        emit(cg, "    cmp rax, 0\n    je .L%d\n", l_else);
        if (then_block)
            emit_node(cg, then_block, has_exit);
        emit(cg, "    jmp .L%d\n", l_end);
        emit(cg, ".L%d:\n", l_else);
        if (else_node)
            emit_node(cg, else_node, has_exit);
        emit(cg, ".L%d:\n", l_end);
        break;
    }
    case NK_WhileStmt: {
        Node *cond = node->children.len > 0 ? node->children.items[0] : NULL;
        Node *body = node->children.len > 1 ? node->children.items[1] : NULL;
        int l_start = new_label(cg);
        int l_end = new_label(cg);
        emit(cg, ".L%d:\n", l_start);
        if (cond) {
            gen_expr(cg, cond);
            emit(cg, "    cmp rax, 0\n    je .L%d\n", l_end);
        }
        if (body)
            emit_node(cg, body, has_exit);
        emit(cg, "    jmp .L%d\n", l_start);
        emit(cg, ".L%d:\n", l_end);
        break;
    }
    case NK_ForStmt: {
        Node *init = node->children.len > 0 ? node->children.items[0] : NULL;
        Node *cond = node->children.len > 1 ? node->children.items[1] : NULL;
        Node *step = node->children.len > 2 ? node->children.items[2] : NULL;
        Node *body = node->children.len > 3 ? node->children.items[3] : NULL;
        scope_push(cg);
        if (init) {
            if (init->kind == NK_LetStmt || init->kind == NK_AssignStmt)
                emit_node(cg, init, has_exit);
            else
                gen_expr(cg, init);
        }
        int l_start = new_label(cg);
        int l_end = new_label(cg);
        emit(cg, ".L%d:\n", l_start);
        if (cond) {
            gen_expr(cg, cond);
            emit(cg, "    cmp rax, 0\n    je .L%d\n", l_end);
        }
        if (body)
            emit_node(cg, body, has_exit);
        if (step) {
            if (step->kind == NK_AssignStmt)
                emit_node(cg, step, has_exit);
            else
                gen_expr(cg, step);
        }
        emit(cg, "    jmp .L%d\n", l_start);
        emit(cg, ".L%d:\n", l_end);
        scope_pop(cg);
        break;
    }
    default:
        fprintf(stderr, "codegen: unsupported node kind %d\n", node->kind);
        exit(1);
    }
}

void codegen_program(Codegen *cg, Node *program) {
    if (!cg || !cg->out) return;

    /* locate the main function */
    Node *main_fn = NULL;
    if (program && program->kind == NK_Program) {
        for (size_t i = 0; i < program->children.len && !main_fn; i++) {
            Node *child = program->children.items[i];
            if (!child) continue;
            if (child->kind == NK_Block) {
                for (size_t j = 0; j < child->children.len; j++) {
                    Node *fn = child->children.items[j];
                    if (fn && fn->kind == NK_FnDecl && fn->value && strcmp(fn->value, "main") == 0) {
                        main_fn = fn;
                        break;
                    }
                }
            } else if (child->kind == NK_FnDecl && child->value && strcmp(child->value, "main") == 0) {
                main_fn = child;
            }
        }
    }

    if (!main_fn) {
        fprintf(stderr, "codegen: no main\n");
        exit(1);
    }

    emit(cg, ".intel_syntax noprefix\n");
    static bool externs_emitted = false;
    if (!externs_emitted) {
        emit(cg, ".extern hsu_print_int\n");
        emit(cg, ".extern hsu_print_cstr\n");
        emit(cg, ".extern hsu_concat\n");
        emit(cg, ".extern exit\n");
        externs_emitted = true;
    }
    emit(cg, ".text\n");
    emit(cg, ".globl main\n");

    bool has_exit = false;
    emit_node(cg, main_fn, &has_exit);

    if (cg->strs.len > 0) {
        emit(cg, ".section .rodata\n");
        emit(cg, ".p2align 4\n");
        for (size_t i = 0; i < cg->strs.len; i++) {
            const char *s = cg->strs.items[i];
            emit(cg, ".Lstr%zu: .asciz \"", i);
            for (const char *p = s; *p; p++) {
                if (*p == '"' || *p == '\\')
                    emit(cg, "\\%c", *p);
                else if (*p == '\n')
                    emit(cg, "\\n");
                else if (*p == '\t')
                    emit(cg, "\\t");
                else if (*p == '\r')
                    emit(cg, "\\r");
                else
                    emit(cg, "%c", *p);
            }
            emit(cg, "\"\n");
        }
    }
}

