#ifndef PARSER_H
#define PARSER_H


#include "lexer.h"

// Kinds of AST nodes that may appear in the syntax tree.
typedef enum {
  NK_Program,
  NK_Block,
  NK_FnDecl,
  NK_ExprStmt,
  NK_LetStmt,
  NK_AssignStmt,
  NK_IfStmt,
  NK_WhileStmt,
  NK_ForStmt,
  NK_WriteStmt,
  NK_ExitStmt,
  NK_Unary,
  NK_Binary,
  NK_Int,
  NK_String,
  NK_Bool,
  NK_Identifier,
} NodeKind;

// Forward declarations
struct Node;
typedef struct Type Type;

// Simple growable array used for storing block children.  It mirrors a
// small subset of functionality of more feature rich vector types.
typedef struct {
  struct Node **items;
  size_t len;
  size_t cap;
} Vec;

// Representation for a single AST node.  Besides the previous value
// and left/right pointers the node now stores its kind, an optional
// operator token and a dynamic array of children for block like nodes.
typedef struct Node {
  NodeKind kind;     // What sort of AST node this is
  TokenType type;    // Original token type (for literals)
  TokenType op;      // Operator token for binary expressions
  char *value;       // Literal lexeme if applicable
  struct Node *right;
  struct Node *left;
  Vec children;      // Used when this node represents a block
  Type *ty;          // Inferred semantic type
} Node;

// Basic initializer for AST nodes.
Node *init_node(Node *node, const char *value, TokenType type);
void free_tree(Node *node);

Node *parser(Token *tokens);
void print_tree(Node *node, int indent);

#endif
