#ifndef SEM_H
#define SEM_H

#include "parser.h"

// Basic type system used by the semantic analyser

typedef enum {
  TY_INT,
  TY_STRING,
  TY_BOOL,
  TY_VOID,
  TY_UNKNOWN
} TypeKind;

typedef struct Type {
  TypeKind kind;
} Type;

// Symbol table entry
typedef struct Binding {
  char *name;
  Type *type;
  struct Binding *next;
} Binding;

// Lexical scope containing variable bindings
typedef struct Scope {
  struct Scope *parent;
  Binding *bindings;
} Scope;

// --- Scope utilities -------------------------------------------------------
Scope *scope_new(Scope *parent);
Type *scope_lookup(Scope *scope, const char *name);
int   scope_insert(Scope *scope, const char *name, Type *type);

// --- Semantic analysis -----------------------------------------------------
Type *sem_expr(Node *node, Scope *scope);
int   sem_block(Node *block, Scope *scope);
void  sem_program(Node *root);

// Helpers to obtain primitive types
Type *type_int(void);
Type *type_string(void);
Type *type_bool(void);
Type *type_void(void);
Type *type_unknown(void);

#endif // SEM_H
