#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"
#include "tools.h"
#include "token_helpers.h"

#define MAX_CURLY_STACK_LENGTH 64

typedef struct {
  Node *content[MAX_CURLY_STACK_LENGTH];
  int top;
} curly_stack;

Node *peek_curly(curly_stack *stack){
  if (stack->top < 0)
    return NULL;
  return stack->content[stack->top];
}

bool push_curly(curly_stack *stack, Node *element){
  if (stack->top + 1 >= MAX_CURLY_STACK_LENGTH)
    return false;
  stack->top++;
  stack->content[stack->top] = element;
  return true;
}

Node *pop_curly(curly_stack *stack){
  if (stack->top < 0)
    return NULL;
  Node *result = stack->content[stack->top];
  stack->top--;
  return result;
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
  return node;
}

// --- tree printer ----------------------------------------------------------
void print_tree(Node *node, int indent, char *identifier, int is_last) {
  if (!node)
    return;

  for (int i = 0; i < indent; i++) {
    printf(" ");
  }
  printf("%s", identifier);
  if (node->value)
    printf(" -> %s", node->value);
  printf("\n");

  indent += 4;
  if (node->left)
    print_tree(node->left, indent, "left", 0);
  if (node->right)
    print_tree(node->right, indent, "right", 0);
  for (size_t i = 0; i < node->children.len; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "child[%zu]", i);
    print_tree(node->children.items[i], indent, buf,
               i == node->children.len - 1);
  }
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
    [NOT] = 80,
    [PLUS_PLUS] = 80,
    [MINUS_MINUS] = 80,
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
  case INT:
  case STRING:
  case BOOL:
  case IDENTIFIER: {
    next(pp);
    Node *node = init_node(NULL, tok->value, tok->type);
    node->kind = NK_Literal;
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
    Node *right = parse_expr(pp, 80);
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
    Node *right = parse_expr(pp, lb);
    Node *node = init_node(NULL, NULL, 0);
    node->kind = NK_Binary;
    node->op = op;
    node->left = left;
    node->right = right;
    left = node;
  }
  return left;
}

Node *handle_exit_syscall(Token **pp, Node *current){
  Token *tok = expect(pp, EXIT, "Invalid Syntax on EXIT");
  Node *exit_node = init_node(NULL, tok->value, EXIT);
  current->right = exit_node;
  current = exit_node;

  expect(pp, OPEN_PAREN, "Invalid Syntax on OPEN");
  Node *expr = parse_expr(pp, 0);
  exit_node->left = expr;
  expect(pp, CLOSE_PAREN, "Invalid Syntax on CLOSE");
  expect(pp, SEMICOLON, "Invalid Syntax on SEMI");

  return current;
}

void handle_token_errors(char *error_text, Token *current_token, bool isType){
  if(current_token->type == END_OF_TOKENS || !isType){
    print_error(error_text, current_token->line_num);
  }
}

Node *create_variable_reusage(Token **pp, Node *current){
  Node *expr = parse_expr(pp, 0);
  expect(pp, SEMICOLON, "Invalid Syntax After Expression");
  current->right = expr;
  return current;
}

Node *create_variables(Token **pp, Node *current){
  Token *token = peek(pp); // at LET
  Node *var_node = init_node(NULL, token->value, token->type);
  current->right = var_node;
  current = var_node;
  next(pp); // consume LET

  // move past identifier and optional initialization until ';'
  while(peek(pp)->type != SEMICOLON && peek(pp)->type != END_OF_TOKENS){
    next(pp);
  }
  if(match(pp, SEMICOLON)){
    // consumed ';'
  }
  return current;
}

Node *handle_write(Token **pp, Node *current){
  expect(pp, WRITE, "Invalid Syntax on WRITE");
  expect(pp, OPEN_PAREN, "Invalid Syntax on OPEN");
  Token *expr_tok = peek(pp);
  handle_token_errors("Invalid Syntax on EXP", expr_tok,
                      expr_tok->type == INT || expr_tok->type == STRING ||
                      expr_tok->type == IDENTIFIER || expr_tok->type == BOOL);
  Node *expr_node = init_node(NULL, expr_tok->value, expr_tok->type);
  next(pp);
  while(peek(pp)->type != CLOSE_PAREN && peek(pp)->type != END_OF_TOKENS){
    next(pp);
  }
  expect(pp, CLOSE_PAREN, "Invalid Syntax on CLOSE");
  expect(pp, SEMICOLON, "Invalid Syntax on SEMI");
  Node *write_node = init_node(NULL, NULL, WRITE);
  write_node->left = expr_node;
  current->right = write_node;
  current = write_node;
  return current;
}

Node *handle_if(Token **pp, Node *current){
  Token *token = peek(pp);
  Node *cond_node = init_node(NULL, token->value, token->type);
  current->right = cond_node;
  current = cond_node;
  next(pp);

  if(cond_node->type == ELSE){
    return current;
  }

  expect(pp, OPEN_PAREN, "Invalid Syntax on OPEN");
  Token *expr_tok = peek(pp);
  handle_token_errors("Invalid Syntax on EXP", expr_tok,
                      expr_tok->type == INT || expr_tok->type == STRING ||
                      expr_tok->type == IDENTIFIER || expr_tok->type == BOOL);
  Node *expr_node = init_node(NULL, expr_tok->value, expr_tok->type);
  current->left = expr_node;
  next(pp);
  while(peek(pp)->type != CLOSE_PAREN && peek(pp)->type != END_OF_TOKENS){
    next(pp);
  }
  expect(pp, CLOSE_PAREN, "Invalid Syntax on CLOSE");
  return current;
}

Node *handle_while(Token **current_token, Node *current){
  return handle_if(current_token, current);
}

Node *handle_for(Token **pp, Node *current){
  Token *token = peek(pp);
  Node *for_node = init_node(NULL, token->value, FOR);
  current->right = for_node;
  current = for_node;
  next(pp);
  expect(pp, OPEN_PAREN, "Invalid Syntax on OPEN");
  while(peek(pp)->type != CLOSE_PAREN && peek(pp)->type != END_OF_TOKENS){
    next(pp);
  }
  expect(pp, CLOSE_PAREN, "Invalid Syntax on CLOSE");
  return current;
}

Node *handle_fn(Token **pp, Node *current){
  Token *token = peek(pp);
  Node *fn_node = init_node(NULL, token->value, FN);
  current->right = fn_node;
  current = fn_node;
  next(pp);

  Token *id_tok = expect(pp, IDENTIFIER, "Invalid Syntax on IDENT");
  Node *identifier_node = init_node(NULL, id_tok->value, IDENTIFIER);
  current->left = identifier_node;

  expect(pp, OPEN_PAREN, "Invalid Syntax on OPEN");
  while(peek(pp)->type != CLOSE_PAREN && peek(pp)->type != END_OF_TOKENS){
    next(pp);
  }
  expect(pp, CLOSE_PAREN, "Invalid Syntax on CLOSE");
  return fn_node;
}
Node *parser(Token *tokens){
  Token *current_token = tokens;
  Node *root = init_node(NULL, NULL, BEGINNING);
  Node *current = root;
  curly_stack *stack = malloc(sizeof(curly_stack));
  stack->top = -1;
  if (!push_curly(stack, root)) {
    printf("ERROR: curly stack overflow\n");
    exit(1);
  }

  bool allow_else = false;
  bool after_if_block = false;

  Token **pp = &current_token;

  while(peek(pp)->type != END_OF_TOKENS){
    Token *tok = peek(pp);
    if(current == NULL){
      break;
    }
    switch(tok->type){
      case LET:
        if(allow_else && after_if_block){ allow_else=false; after_if_block=false; }
        current = create_variables(pp, current);
        break;
      case FN:
        if(allow_else && after_if_block){ allow_else=false; after_if_block=false; }
        current = handle_fn(pp, current);
        break;
      case FOR:
        if(allow_else && after_if_block){ allow_else=false; after_if_block=false; }
        current = handle_for(pp, current);
        break;
      case WHILE:
        if(allow_else && after_if_block){ allow_else=false; after_if_block=false; }
        current = handle_while(pp, current);
        break;
      case WRITE:
        if(allow_else && after_if_block){ allow_else=false; after_if_block=false; }
        current = handle_write(pp, current);
        break;
      case EXIT:
        if(allow_else && after_if_block){ allow_else=false; after_if_block=false; }
        current = handle_exit_syscall(pp, current);
        break;
      case IF:
        current = handle_if(pp, current);
        allow_else = true;
        after_if_block = false;
        break;
      case ELSE_IF:
        if(!(allow_else && after_if_block)){
          print_error("Unexpected else without matching if", tok->line_num);
        }
        current = handle_if(pp, current);
        allow_else = true;
        after_if_block = false;
        break;
      case ELSE:
        if(!(allow_else && after_if_block)){
          print_error("Unexpected else without matching if", tok->line_num);
        }
        current = handle_if(pp, current);
        allow_else = false;
        after_if_block = false;
        break;
      case OPEN_CURLY:
        if (!push_curly(stack, current)) {
          printf("ERROR: curly stack overflow\n");
          exit(1);
        }
        next(pp);
        break;
      case CLOSE_CURLY: {
        Node *popped = pop_curly(stack);
        if(popped == NULL){
          printf("ERROR: Expected Open Parenthesis!\n");
          exit(1);
        }
        if(allow_else){ after_if_block = true; }
        next(pp);
        break; }
      case IDENTIFIER:
        if(allow_else && after_if_block){ allow_else=false; after_if_block=false; }
        current = create_variable_reusage(pp, current);
        break;
      default:
        if(allow_else && after_if_block){ allow_else=false; after_if_block=false; }
        next(pp);
        break;
    }
  }
  return root;
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
