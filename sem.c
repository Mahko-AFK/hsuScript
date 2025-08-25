#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sem.h"

// --- primitive type singletons -------------------------------------------
static Type TY_INT_OBJ    = { TY_INT };
static Type TY_STRING_OBJ = { TY_STRING };
static Type TY_BOOL_OBJ   = { TY_BOOL };
static Type TY_VOID_OBJ   = { TY_VOID };
static Type TY_UNKNOWN_OBJ= { TY_UNKNOWN };

Type *type_int(void)    { return &TY_INT_OBJ; }
Type *type_string(void) { return &TY_STRING_OBJ; }
Type *type_bool(void)   { return &TY_BOOL_OBJ; }
Type *type_void(void)   { return &TY_VOID_OBJ; }
Type *type_unknown(void){ return &TY_UNKNOWN_OBJ; }

// --- scope handling -------------------------------------------------------
Scope *scope_new(Scope *parent) {
  Scope *s = malloc(sizeof(Scope));
  if (!s) { perror("scope_new"); exit(1); }
  s->parent = parent;
  s->bindings = NULL;
  return s;
}

static Binding *binding_new(const char *name, Type *type) {
  Binding *b = malloc(sizeof(Binding));
  if (!b) { perror("binding_new"); exit(1); }
  b->name = strdup(name);
  b->type = type;
  b->next = NULL;
  return b;
}

Type *scope_lookup(Scope *scope, const char *name) {
  for (Scope *s = scope; s; s = s->parent) {
    for (Binding *b = s->bindings; b; b = b->next) {
      if (strcmp(b->name, name) == 0)
        return b->type;
    }
  }
  return NULL;
}

int scope_insert(Scope *scope, const char *name, Type *type) {
  for (Binding *b = scope->bindings; b; b = b->next) {
    if (strcmp(b->name, name) == 0)
      return 0; // duplicate
  }
  Binding *b = binding_new(name, type);
  b->next = scope->bindings;
  scope->bindings = b;
  return 1;
}

// --- semantic helpers -----------------------------------------------------
static void sem_error(const char *msg, const char *name) {
  if (name)
    fprintf(stderr, "Semantic error: %s '%s'\n", msg, name);
  else
    fprintf(stderr, "Semantic error: %s\n", msg);
  exit(1);
}

Type *sem_expr(Node *node, Scope *scope) {
  if (!node) return type_void();
  switch (node->kind) {
  case NK_Int:
    return node->ty = type_int();
  case NK_String:
    return node->ty = type_string();
  case NK_Bool:
    return node->ty = type_bool();
  case NK_Identifier: {
    Type *t = scope_lookup(scope, node->value);
    if (!t) sem_error("undeclared identifier", node->value);
    return node->ty = t;
  }
  case NK_Unary: {
    Type *rt = sem_expr(node->left, scope);
    switch (node->op) {
    case NOT:
      if (rt != type_bool()) sem_error("operator requires boolean", NULL);
      node->ty = type_bool();
      return node->ty;
    case DASH:
    case PLUS:
    case PLUS_PLUS:
    case MINUS_MINUS:
      if (rt != type_int()) sem_error("operator requires integer", NULL);
      node->ty = type_int();
      return node->ty;
    default:
      node->ty = rt;
      return node->ty;
    }
  }
  case NK_Assign: {
    Type *lt = sem_expr(node->left, scope);
    Type *rt = sem_expr(node->right, scope);
    if (node->op == PLUS_EQUALS || node->op == MINUS_EQUALS) {
      if (lt != type_int() || rt != type_int())
        sem_error("compound assignment on non-integers", NULL);
    }
    if (lt != rt)
      sem_error("assignment of incompatible types", NULL);
    node->ty = lt;
    return node->ty;
  }
  case NK_Binary: {
    Type *lt = sem_expr(node->left, scope);
    Type *rt = sem_expr(node->right, scope);
    switch (node->op) {
    case PLUS:
    case DASH:
    case STAR:
    case SLASH:
    case PERCENT:
      if (lt != type_int() || rt != type_int())
        sem_error("arithmetic on non-integers", NULL);
      node->ty = type_int();
      return node->ty;
    case EQUALS:
    case NOT_EQUALS:
    case LESS:
    case LESS_EQUALS:
    case GREATER:
    case GREATER_EQUALS:
      if (lt != rt) sem_error("comparison of incompatible types", NULL);
      node->ty = type_bool();
      return node->ty;
    case AND:
    case OR:
      if (lt != type_bool() || rt != type_bool())
        sem_error("logical operator on non-bools", NULL);
      node->ty = type_bool();
      return node->ty;
    default:
      node->ty = type_unknown();
      return node->ty;
    }
  }
  default:
    node->ty = type_void();
    return node->ty;
  }
}

static int sem_if(Node *ifnode, Scope *scope);
static void sem_for(Node *fornode, Scope *scope);

static void sem_let(Node *stmt, Scope *scope) {
  const char *name = stmt->value;
  Type *t = type_unknown();
  if (stmt->right)
    t = sem_expr(stmt->right, scope);
  if (!scope_insert(scope, name, t))
    sem_error("duplicate identifier", name);
  stmt->ty = type_void();
}

static void sem_assign(Node *stmt, Scope *scope) {
  const char *name = NULL;
  if (stmt->left && stmt->left->value)
    name = stmt->left->value;
  else
    sem_error("assignment missing identifier", NULL);
  Type *lhs = scope_lookup(scope, name);
  if (!lhs) sem_error("undeclared identifier", name);
  Type *rhs = sem_expr(stmt->right, scope);
  if (lhs != rhs) sem_error("assignment of incompatible types", name);
  stmt->ty = lhs;
}

int sem_block(Node *block, Scope *scope) {
  Scope *inner = scope_new(scope);
  int must_exit = 0;
  for (size_t i = 0; i < block->children.len; i++) {
    if (must_exit)
      break;
    Node *stmt = block->children.items[i];
    switch (stmt->kind) {
    case NK_LetStmt:
      sem_let(stmt, inner);
      break;
    case NK_AssignStmt:
      sem_assign(stmt, inner);
      break;
    case NK_ExprStmt:
      sem_expr(stmt->left, inner);
      stmt->ty = type_void();
      break;
    case NK_WriteStmt:
      sem_expr(stmt->left, inner);
      stmt->ty = type_void();
      break;
    case NK_ExitStmt:
      if (sem_expr(stmt->left, inner) != type_int())
        sem_error("exit expects integer status", NULL);
      stmt->ty = type_void();
      must_exit = 1;
      break;
    case NK_IfStmt:
      if (sem_if(stmt, inner))
        must_exit = 1;
      stmt->ty = type_void();
      break;
    case NK_Block:
      if (sem_block(stmt, inner))
        must_exit = 1;
      stmt->ty = type_void();
      break;
    case NK_ForStmt:
      sem_for(stmt, inner);
      stmt->ty = type_void();
      break;
    default:
      sem_expr(stmt, inner);
      stmt->ty = type_void();
      break;
    }
  }
  return must_exit;
}

static int sem_if(Node *ifnode, Scope *scope) {
  Node *cond = ifnode->left;
  Node *then_block = ifnode->right;
  if (sem_expr(cond, scope) != type_bool())
    sem_error("if condition must be boolean", NULL);
  int exit_then = sem_block(then_block, scope);
  if (ifnode->children.len > 0) {
    Node *else_node = ifnode->children.items[0];
    int exit_alt;
    if (else_node->kind == NK_IfStmt)
      exit_alt = sem_if(else_node, scope);
    else
      exit_alt = sem_block(else_node, scope);
    return exit_then && exit_alt;
  }
  return 0;
}

static void sem_for(Node *fornode, Scope *scope) {
  Scope *loop = scope_new(scope);
  Node *init = fornode->children.len > 0 ? fornode->children.items[0] : NULL;
  Node *cond = fornode->children.len > 1 ? fornode->children.items[1] : NULL;
  Node *step = fornode->children.len > 2 ? fornode->children.items[2] : NULL;
  Node *body = fornode->children.len > 3 ? fornode->children.items[3] : NULL;

  if (init) {
    if (init->kind == NK_LetStmt)
      sem_let(init, loop);
    else if (init->kind == NK_AssignStmt)
      sem_assign(init, loop);
    else
      sem_expr(init, loop);
  }

  if (cond) {
    if (sem_expr(cond, loop) != type_bool())
      sem_error("for condition must be boolean", NULL);
  }

  if (body)
    sem_block(body, loop);

  if (step) {
    if (step->kind == NK_AssignStmt)
      sem_assign(step, loop);
    else
      sem_expr(step, loop);
  }
}

void sem_program(Node *root) {
  if (!root || root->children.len == 0) return;
  Scope *global = scope_new(NULL);
  for (size_t i = 0; i < root->children.len; i++)
    sem_block(root->children.items[i], global);
}

