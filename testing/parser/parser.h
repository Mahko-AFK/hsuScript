#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef struct Node {
  char *value;
  TokenType type;
  struct Node *left;
  struct Node *right;
} Node;

Node *parser(Token *tokens);
void print_tree(Node *node, int indent, char *identifier, int is_last);
void free_tree(Node *node);

#endif
