#ifndef PARSER_H
#define PARSER_H


#include "lexer.h"

// Kinds of AST nodes that may appear in the syntax tree.  The
// enumeration purposefully covers high level program structure
// (program and block), different statement kinds, literals and
// generic operators.
typedef enum {
  NODE_PROGRAM,
  NODE_BLOCK,
  NODE_WRITE_STMT,
  NODE_EXIT_STMT,
  NODE_BINARY_OP,
  NODE_LITERAL,
} NodeKind;

// Forward declaration so we can reference Node inside Vec before the
// full structure is defined.
struct Node;

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
} Node;

// Helper constructors used by the parser to create nodes of the
// various kinds.  They allocate memory with calloc and copy strings
// using strdup.
Node *make_program(void);
Node *make_block(void);
Node *make_binary(TokenType op, Node *lhs, Node *rhs);
Node *make_write(Node *expr);
Node *make_literal(TokenType type, const char *value);

Node *parser(Token *tokens);
void print_tree(Node *node, int indent, char *identifier, int is_last);

#endif
