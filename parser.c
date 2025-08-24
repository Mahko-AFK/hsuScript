#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "parser.h"
#include "tools.h"
#include "token_helpers.h"

#define PREC_PREFIX 90

static const char *kind_name(NodeKind kind) {
  switch (kind) {
  case NK_Program:
    return "Program";
  case NK_Block:
    return "Block";
  case NK_FnDecl:
    return "FnDecl";
  case NK_ExprStmt:
    return "ExprStmt";
  case NK_LetStmt:
    return "LetStmt";
  case NK_AssignStmt:
    return "AssignStmt";
  case NK_IfStmt:
    return "IfStmt";
  case NK_WhileStmt:
    return "WhileStmt";
  case NK_ForStmt:
    return "ForStmt";
  case NK_WriteStmt:
    return "WriteStmt";
  case NK_ExitStmt:
    return "ExitStmt";
  case NK_Unary:
    return "Unary";
  case NK_Binary:
    return "Binary";
  case NK_Assign:
    return "Assign";
  case NK_Int:
    return "Int";
  case NK_String:
    return "String";
  case NK_Bool:
    return "Bool";
  case NK_Identifier:
    return "Identifier";
  default:
    return "Unknown";
  }
}

static const char *op_name(TokenType type) {
  switch (type) {
  case ASSIGNMENT:
    return "=";
  case PLUS_EQUALS:
    return "+=";
  case MINUS_EQUALS:
    return "-=";
  case OR:
    return "||";
  case AND:
    return "&&";
  case EQUALS:
    return "==";
  case NOT_EQUALS:
    return "!=";
  case LESS:
    return "<";
  case LESS_EQUALS:
    return "<=";
  case GREATER:
    return ">";
  case GREATER_EQUALS:
    return ">=";
  case PLUS:
    return "+";
  case DASH:
    return "-";
  case STAR:
    return "*";
  case SLASH:
    return "/";
  case PERCENT:
    return "%";
  case NOT:
    return "!";
  case PLUS_PLUS:
    return "++";
  case MINUS_MINUS:
    return "--";
  default:
    return "?";
  }
}

// --- helper constructors ---------------------------------------------------
// Allocate and initialize a node. `node` is ignored and kept only for
// compatibility with existing call sites.
Node *init_node(Node *node, const char *value, TokenType type) {
  (void)node;
  node = malloc(sizeof(Node));
  if (!node)
    return NULL;
  node->kind = 0;
  node->type = type;
  node->op = 0;
  node->value = value ? strdup(value) : NULL;
  node->left = NULL;
  node->right = NULL;
  node->children.items = NULL;
  node->children.len = 0;
  node->children.cap = 0;
  node->ty = NULL;
  return node;
}

// --- tree printer ----------------------------------------------------------
void print_tree(Node *node, int indent) {
  if (!node)
    return;

  for (int i = 0; i < indent; i++)
    printf(" ");

  printf("%s", kind_name(node->kind));

  if (node->op || node->kind == NK_Assign || node->kind == NK_AssignStmt)
    printf(" op=%s", op_name(node->op));

  if (node->value)
    printf(" value=%s", node->value);

  printf("\n");

  indent += 4;

  if (node->left)
    print_tree(node->left, indent);
  if (node->right)
    print_tree(node->right, indent);
  for (size_t i = 0; i < node->children.len; i++)
    print_tree(node->children.items[i], indent);
}

void print_error(char *error_type, size_t line_number){
  printf("ERROR: %s on line number: %zu\n", error_type, line_number);
  exit(1);
}

// --- expression parsing (Pratt parser) ------------------------------------

// Left binding power table for operators
static const int lbp_table[END_OF_TOKENS + 1] = {
    [ASSIGNMENT] = 10,
    [PLUS_EQUALS] = 10,
    [MINUS_EQUALS] = 10,
    [OR] = 20,
    [AND] = 30,
    [EQUALS] = 40,
    [NOT_EQUALS] = 40,
    [LESS] = 50,
    [LESS_EQUALS] = 50,
    [GREATER] = 50,
    [GREATER_EQUALS] = 50,
    [PLUS] = 60,
    [DASH] = 60,
    [STAR] = 70,
    [SLASH] = 70,
    [PERCENT] = 70,
    [NOT] = PREC_PREFIX,
    [PLUS_PLUS] = PREC_PREFIX,
    [MINUS_MINUS] = PREC_PREFIX,
};

static int lbp(TokenType type) {
  if (type < 0 || type > END_OF_TOKENS)
    return 0;
  return lbp_table[type];
}

static Node *parse_expr(Token **pp, int minbp); // forward declaration

static Node *nud(Token **pp) {
  Token *tok = peek(pp);
  switch (tok->type) {
  case INT: {
    next(pp);
    Node *node = init_node(NULL, tok->value, tok->type);
    node->kind = NK_Int;
    return node;
  }
  case STRING: {
    next(pp);
    Node *node = init_node(NULL, tok->value, tok->type);
    node->kind = NK_String;
    return node;
  }
  case BOOL: {
    next(pp);
    Node *node = init_node(NULL, tok->value, tok->type);
    node->kind = NK_Bool;
    return node;
  }
  case IDENTIFIER: {
    next(pp);
    Node *node = init_node(NULL, tok->value, tok->type);
    node->kind = NK_Identifier;
    return node;
  }
  case OPEN_PAREN: {
    next(pp);
    Node *expr = parse_expr(pp, 0);
    expect(pp, CLOSE_PAREN, "Invalid Syntax on CLOSE");
    return expr;
  }
  case NOT:
  case DASH:
  case PLUS:
  case PLUS_PLUS:
  case MINUS_MINUS: {
    TokenType op = tok->type;
    next(pp);
    Node *right = parse_expr(pp, PREC_PREFIX);
    Node *node = init_node(NULL, NULL, 0);
    node->kind = NK_Unary;
    node->op = op;
    node->left = right;
    return node;
  }
  default:
    print_error("Unexpected token", tok->line_num);
    return NULL;
  }
}

static Node *parse_expr(Token **pp, int minbp) {
  Node *left = nud(pp);
  for (;;) {
    Token *tok = peek(pp);
    int lb = lbp(tok->type);
    if (lb <= minbp)
      break;
    TokenType op = tok->type;
    next(pp);
    // Handle post-increment/decrement used without a right operand.
    TokenType next_type = peek(pp)->type;
    if ((op == PLUS_PLUS || op == MINUS_MINUS) &&
        (next_type == SEMICOLON || next_type == CLOSE_PAREN ||
         next_type == CLOSE_CURLY || next_type == CLOSE_BRACKET ||
         next_type == COMMA || next_type == END_OF_TOKENS)) {
      Node *node = init_node(NULL, NULL, 0);
      node->kind = NK_Unary;
      node->op = op;
      node->left = left;
      left = node;
      continue;
    }

    int rb = lb - (op == ASSIGNMENT || op == PLUS_EQUALS || op == MINUS_EQUALS);
    Node *right = parse_expr(pp, rb);
    Node *node = init_node(NULL, NULL, 0);
    if (op == ASSIGNMENT || op == PLUS_EQUALS || op == MINUS_EQUALS)
      node->kind = NK_Assign;
    else
      node->kind = NK_Binary;
    node->op = op;
    node->left = left;
    node->right = right;
    left = node;
  }
  return left;
}

// --- small vector helper ---------------------------------------------------
static void vec_push(Vec *v, Node *n) {
  if (v->len + 1 > v->cap) {
    v->cap = v->cap ? v->cap * 2 : 4;
    v->items = realloc(v->items, v->cap * sizeof(Node *));
  }
  v->items[v->len++] = n;
}

// --- parsing helpers -------------------------------------------------------

static Node *parse_block(Token **pp);
static Node *parse_stmt(Token **pp);

static Node *parse_write(Token **pp) {
  expect(pp, WRITE, "expected write");
  Node *node = init_node(NULL, NULL, 0);
  node->kind = NK_WriteStmt;
  expect(pp, OPEN_PAREN, "expected (");
  Node *expr = parse_expr(pp, 0);
  node->left = expr;
  expect(pp, CLOSE_PAREN, "expected )");
  expect(pp, SEMICOLON, "expected semicolon");
  return node;
}

static Node *parse_exit(Token **pp) {
  expect(pp, EXIT, "expected exit");
  Node *node = init_node(NULL, NULL, 0);
  node->kind = NK_ExitStmt;
  expect(pp, OPEN_PAREN, "expected (");
  Node *expr = parse_expr(pp, 0);
  node->left = expr;
  expect(pp, CLOSE_PAREN, "expected )");
  expect(pp, SEMICOLON, "expected semicolon");
  return node;
}

static Node *parse_let(Token **pp, bool expect_semi) {
  expect(pp, LET, "expected let");
  Token *id = expect(pp, IDENTIFIER, "expected identifier");
  Node *node = init_node(NULL, id->value, 0);
  node->kind = NK_LetStmt;
  if (match(pp, ASSIGNMENT)) {
    Node *expr = parse_expr(pp, 0);
    node->right = expr;
  }
  if (expect_semi)
    expect(pp, SEMICOLON, "expected semicolon");
  return node;
}

static Node *parse_expr_or_assign_nosemi(Token **pp) {
  Node *expr = parse_expr(pp, 0);
  if (expr && expr->kind == NK_Assign && expr->left &&
      expr->left->kind == NK_Identifier) {
    Node *node = init_node(NULL, NULL, 0);
    node->kind = NK_AssignStmt;
    node->op = expr->op;
    node->left = expr->left;
    node->right = expr->right;
    node->value = strdup(expr->left->value);
    free(expr);
    return node;
  }
  return expr;
}

static Node *parse_assign_or_expr(Token **pp) {
  Node *expr = parse_expr_or_assign_nosemi(pp);
  if (expr && expr->kind == NK_AssignStmt)
    return expr;
  Node *node = init_node(NULL, NULL, 0);
  node->kind = NK_ExprStmt;
  node->left = expr;
  return node;
}

static Node *parse_if_internal(Token **pp, bool consumed_kw) {
  if (!consumed_kw)
    expect(pp, IF, "expected if");
  Node *node = init_node(NULL, NULL, 0);
  node->kind = NK_IfStmt;
  expect(pp, OPEN_PAREN, "expected (");
  Node *cond = parse_expr(pp, 0);
  expect(pp, CLOSE_PAREN, "expected )");
  Node *then_block = parse_block(pp);
  node->left = cond;
  node->right = then_block;
  if (match(pp, ELSE_IF)) {
    Node *elif = parse_if_internal(pp, true);
    vec_push(&node->children, elif);
  } else if (match(pp, ELSE)) {
    Node *else_block = parse_block(pp);
    vec_push(&node->children, else_block);
  }
  return node;
}

static Node *parse_if(Token **pp) { return parse_if_internal(pp, false); }

static Node *parse_while(Token **pp) {
  expect(pp, WHILE, "expected while");
  Node *node = init_node(NULL, NULL, 0);
  node->kind = NK_WhileStmt;
  expect(pp, OPEN_PAREN, "expected (");
  Node *cond = parse_expr(pp, 0);
  expect(pp, CLOSE_PAREN, "expected )");
  Node *body = parse_block(pp);
  vec_push(&node->children, cond);
  vec_push(&node->children, body);
  return node;
}

static Node *parse_for(Token **pp) {
  expect(pp, FOR, "expected for");
  Node *node = init_node(NULL, NULL, 0);
  node->kind = NK_ForStmt;
  expect(pp, OPEN_PAREN, "expected (");

  Node *init = NULL;
  if (peek(pp)->type != SEMICOLON) {
    if (peek(pp)->type == LET)
      init = parse_let(pp, false);
    else {
      init = parse_assign_or_expr(pp);
    }
  }
  expect(pp, SEMICOLON, "expected semicolon");
  vec_push(&node->children, init);

  Node *cond = NULL;
  if (peek(pp)->type != SEMICOLON)
    cond = parse_expr(pp, 0);
  expect(pp, SEMICOLON, "expected semicolon");
  vec_push(&node->children, cond);

  Node *step = NULL;
  if (peek(pp)->type != CLOSE_PAREN) {
    step = parse_expr_or_assign_nosemi(pp);
  }
  expect(pp, CLOSE_PAREN, "expected )");
  vec_push(&node->children, step);

  Node *body = parse_block(pp);
  vec_push(&node->children, body);
  return node;
}

static Node *parse_fn(Token **pp) {
  expect(pp, FN, "expected fn");
  Token *id = expect(pp, IDENTIFIER, "expected identifier");
  Node *node = init_node(NULL, id->value, FN);
  node->kind = NK_FnDecl;
  expect(pp, OPEN_PAREN, "expected (");
  while (peek(pp)->type != CLOSE_PAREN && peek(pp)->type != END_OF_TOKENS)
    next(pp);
  expect(pp, CLOSE_PAREN, "expected )");
  Node *body = parse_block(pp);
  vec_push(&node->children, body);
  return node;
}

static Node *parse_block(Token **pp) {
  expect(pp, OPEN_CURLY, "expected {");
  Node *block = init_node(NULL, NULL, 0);
  block->kind = NK_Block;
  while (peek(pp)->type != CLOSE_CURLY && peek(pp)->type != END_OF_TOKENS) {
    Node *stmt = parse_stmt(pp);
    if (stmt)
      vec_push(&block->children, stmt);
  }
  expect(pp, CLOSE_CURLY, "expected }");
  return block;
}

static Node *parse_stmt(Token **pp) {
  switch (peek(pp)->type) {
  case WRITE:
    return parse_write(pp);
  case EXIT:
    return parse_exit(pp);
  case LET:
    return parse_let(pp, true);
  case IF:
    return parse_if(pp);
  case WHILE:
    return parse_while(pp);
  case FOR:
    return parse_for(pp);
  case FN:
    return parse_fn(pp);
  case OPEN_CURLY:
    return parse_block(pp);
  default: {
    Node *stmt = parse_assign_or_expr(pp);
    expect(pp, SEMICOLON, "expected semicolon");
    return stmt;
  }
  }
}

Node *parser(Token *tokens) {
  Token *t = tokens;
  Token **pp = &t;
  Node *program = init_node(NULL, NULL, 0);
  program->kind = NK_Program;
  Node *block = init_node(NULL, NULL, 0);
  block->kind = NK_Block;
  while (peek(pp)->type != END_OF_TOKENS) {
    Node *stmt = parse_stmt(pp);
    if (stmt)
      vec_push(&block->children, stmt);
  }
  vec_push(&program->children, block);
  return program;
}

void free_tree(Node *node) {
  if (!node)
    return;
  free_tree(node->left);
  free_tree(node->right);
  for (size_t i = 0; i < node->children.len; i++) {
    free_tree(node->children.items[i]);
  }
  free(node->children.items);
  free(node->value);
  free(node);
}
