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

Node *parse_expression(Token **pp){
  Token *current_token = peek(pp);
  Node *expr_node = init_node(NULL, current_token->value, current_token->type);
  next(&current_token);
  if(!is_operator(peek(&current_token)->type)){
    *pp = current_token;
    return expr_node;
  }
  *pp = current_token;
  return expr_node;
}

void generate_operation_nodes(Token **pp, Node *current_node){
  Token *current_token = peek(pp);
  Node *oper_node = init_node(NULL, current_token->value, current_token->type);
  current_node->left->left = oper_node;
  current_node = oper_node;
  prev(&current_token);
  if(current_token->type == INT){
    Node *expr_node = init_node(NULL, current_token->value, INT);
    current_node->left = expr_node;
  } else if(current_token->type == IDENTIFIER){
    Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
    current_node->left = identifier_node;
  } else {
    printf("ERROR: expected int or identifier\n");
    exit(1);
  }
  next(&current_token);
  next(&current_token);
  while(current_token->type == INT || current_token->type == IDENTIFIER || is_operator(current_token->type)){
    if(current_token->type == INT || current_token->type == IDENTIFIER){
      if((current_token->type != INT && current_token->type != IDENTIFIER) || current_token == NULL){
        printf("Syntax Error hERE\n");
        exit(1);
      }
      next(&current_token);
      if(!is_operator(current_token->type)){
        prev(&current_token);
        if(current_token->type == INT){
          Node *second_expr_node = init_node(NULL, current_token->value, INT);
          current_node->right = second_expr_node;
        } else if(current_token->type == IDENTIFIER){
          Node *second_identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
          current_node->right = second_identifier_node;
        } else {
          printf("ERROR: Expected Integer or Identifier\n");
          exit(1);
        }
      }
    }
    if(is_operator(current_token->type)){
      Node *next_oper_node = init_node(NULL, current_token->value, current_token->type);
      current_node->right = next_oper_node;
      current_node = next_oper_node;
      prev(&current_token);
      if(current_token->type == INT){
        Node *second_expr_node = init_node(NULL, current_token->value, INT);
        current_node->left = second_expr_node;
      } else if(current_token->type == IDENTIFIER){
        Node *second_identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
        current_node->left = second_identifier_node;
      } else {
        printf("ERROR: Expected IDENTIFIER or INT\n");
        exit(1);
      }
      next(&current_token);
    }
    next(&current_token);
  }
  *pp = current_token;
}

Node *handle_exit_syscall(Token **pp, Node *current){
    Token *current_token = peek(pp);
    Node *exit_node = init_node(NULL, current_token->value, EXIT);
    current->right = exit_node;
    current = exit_node;
    next(&current_token);

    if(current_token->type == END_OF_TOKENS){
      print_error("Invalid Syntax on OPEN", current_token->line_num);
    }
    if(current_token->type == OPEN_PAREN){
      Node *open_paren_node = init_node(NULL, current_token->value, OPEN_PAREN);
      current->left = open_paren_node;
      next(&current_token);
      if(current_token->type == END_OF_TOKENS){
        print_error("Invalid Syntax on INT", current_token->line_num);
      }
      if(current_token->type == INT || current_token->type == IDENTIFIER){
        next(&current_token);
        if(is_operator(current_token->type) && current_token != NULL){
          generate_operation_nodes(&current_token, current);
          prev(&current_token);
        } else {
          prev(&current_token);
          Node *expr_node = init_node(NULL, current_token->value, current_token->type);
          current->left->left = expr_node;
        }
        next(&current_token);
        if(current_token->type == END_OF_TOKENS){
          print_error("Invalid Syntax on cLOSE", current_token->line_num);
        }
        if(current_token->type == CLOSE_PAREN && current_token->type != END_OF_TOKENS){
          Node *close_paren_node = init_node(NULL, current_token->value, CLOSE_PAREN);
          current->left->right = close_paren_node;
          next(&current_token);
          if(current_token->type == END_OF_TOKENS){
            print_error("Invalid Syntax on SEMI", current_token->line_num);
          }
          if(current_token->type == SEMICOLON){
            Node *semi_node = init_node(NULL, current_token->value, SEMICOLON);
            current->right = semi_node;
            current = semi_node;
          } else {
            print_error("Invalid Syntax on SEMI", current_token->line_num);
          }
        } else {
            print_error("Invalid Syntax on CLOSE", current_token->line_num);
        }
      } else {
        print_error("Invalid Syntax INT", current_token->line_num);
      }
  } else {
    print_error("Invalid Syntax OPEN", current_token->line_num);
  }
  *pp = current_token;
  return current;
}

void handle_token_errors(char *error_text, Token *current_token, bool isType){
  if(current_token->type == END_OF_TOKENS || !isType){
    print_error(error_text, current_token->line_num);
  }
}

Node *create_variable_reusage(Token **pp, Node *current){
  Token *current_token = peek(pp);
  Node *main_identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
  current->left = main_identifier_node;
  current = main_identifier_node;
  next(&current_token);

  handle_token_errors("Invalid syntax after idenfitier", current_token, is_operator(current_token->type));

  if(is_operator(current_token->type)){
    if(current_token->type == ASSIGNMENT){
      print_error("Invalid Variable Syntax on =", current_token->line_num);
    }
    Node *equals_node = init_node(NULL, current_token->value, current_token->type);
    current->left = equals_node;
    current = equals_node;
    next(&current_token);
  }
  if(current_token->type == END_OF_TOKENS){
    print_error("Invalid Syntax After Equals", current_token->line_num);
  }

  next(&current_token);
  if(is_operator(current_token->type)){
    Node *oper_node = init_node(NULL, current_token->value, current_token->type);
    current->left = oper_node;
    current = oper_node;
    prev(&current_token);
    if(current_token->type == INT){
      Node *expr_node = init_node(NULL, current_token->value, INT);
      oper_node->left = expr_node;
      next(&current_token);
      next(&current_token);
    } else if(current_token->type == IDENTIFIER){
      Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
      oper_node->left = identifier_node;
      next(&current_token);
      next(&current_token);
    } else {
      print_error("ERROR: Expected IDENTIFIER or INT", current_token->line_num);
    }
    next(&current_token);

    if(is_operator(current_token->type)){
      Node *oper_node2 = init_node(NULL, current_token->value, current_token->type);
      current->right = oper_node2;
      current = oper_node2;
      int operation = 1;
      prev(&current_token);
      prev(&current_token);
      while(operation){
        next(&current_token);
        if(current_token->type == INT){
          Node *expr_node = init_node(NULL, current_token->value, INT);
          current->left = expr_node;
        } else if(current_token->type == IDENTIFIER){
          Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
          current->left = identifier_node;
        } else {
          print_error("ERROR: Unexpected Token\n", current_token->line_num);
          exit(1);
        }
        next(&current_token);
        if(is_operator(current_token->type)){
          next(&current_token);
          next(&current_token);
          if(!is_operator(current_token->type)){
            prev(&current_token);
            if(current_token->type == INT){
              Node *expr_node = init_node(NULL, current_token->value, INT);
              current->right = expr_node;
              next(&current_token);
            } else if(current_token->type == IDENTIFIER){
              Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
              current->right = identifier_node;
              next(&current_token);
            } else {
              printf("ERROR: UNRECOGNIZED TOKEN!\n");
              exit(1);
            }
            operation = 0;
          } else {
            prev(&current_token);
            prev(&current_token);
            Node *oper_node3 = init_node(NULL, current_token->value, current_token->type);
            current->right = oper_node3;
            current = oper_node3;
          }
        } else {
          operation = 0;
        }
      }
    } else {
      prev(&current_token);
      if(current_token->type == INT){
        Node *expr_node = init_node(NULL, current_token->value, INT);
        oper_node->right = expr_node;
      } else if(current_token->type == IDENTIFIER){
        Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
        oper_node->right = identifier_node;
      } else if(current_token->type == STRING){
        Node *string_node = init_node(NULL, current_token->value, STRING);
        oper_node->right = string_node;
      }
      next(&current_token);
    }
  } else {
    prev(&current_token);
    if(current_token->type == INT){
      Node *expr_node = init_node(NULL, current_token->value, INT);
      current->left = expr_node;
      next(&current_token);
    } else if(current_token->type == IDENTIFIER){
      Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
      current->left = identifier_node;
      next(&current_token);
    } else if(current_token->type == STRING){
      Node *string_node = init_node(NULL, current_token->value, STRING);
      current->left = string_node;
      next(&current_token);
    }
  }
  handle_token_errors("Invalid Syntax After Expression", current_token, is_separator(current_token->type));

  current = main_identifier_node;
  if(current_token->type == SEMICOLON){
    Node *semi_node = init_node(NULL, current_token->value, SEMICOLON);
    current->right = semi_node;
    current = semi_node;
  }
  *pp = current_token;
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
  Node *open_paren_node = init_node(NULL, "(", OPEN_PAREN);
  current->left = open_paren_node;
  open_paren_node->left = expr_node;
  next(pp);
  while(peek(pp)->type != CLOSE_PAREN && peek(pp)->type != END_OF_TOKENS){
    next(pp);
  }
  expect(pp, CLOSE_PAREN, "Invalid Syntax on CLOSE");
  Node *close_paren_node = init_node(NULL, ")", CLOSE_PAREN);
  open_paren_node->right = close_paren_node;
  current = close_paren_node;
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
  Node *open_paren_node = init_node(NULL, "(", OPEN_PAREN);
  identifier_node->left = open_paren_node;

  expect(pp, CLOSE_PAREN, "Invalid Syntax on CLOSE");
  Node *close_paren_node = init_node(NULL, ")", CLOSE_PAREN);
  open_paren_node->right = close_paren_node;
  current = close_paren_node;
  return current;
}
Node *parser(Token *tokens){
  Token *current_token = tokens;
  Node *root = init_node(NULL, NULL, BEGINNING);
  Node *current = root;
  Node *open_curly = NULL;

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
        open_curly = init_node(NULL, tok->value, tok->type);
        current->left = open_curly;
        if (!push_curly(stack, open_curly)) {
          printf("ERROR: curly stack overflow\n");
          exit(1);
        }
        current = open_curly;
        next(pp);
        break;
      case CLOSE_CURLY: {
        Node *close_curly = init_node(NULL, tok->value, tok->type);
        open_curly = pop_curly(stack);
        if(open_curly == NULL){
          printf("ERROR: Expected Open Parenthesis!\n");
          exit(1);
        }
        current->right = close_curly;
        current = close_curly;
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
