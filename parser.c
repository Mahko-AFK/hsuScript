#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parser.h"
#include "tools.h"

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

Node *parse_expression(Token *current_token){
  Node *expr_node = init_node(NULL, current_token->value, current_token->type);
  current_token++;
  if(!is_operator(current_token->type)){
    return expr_node;
  }
  return expr_node;
}

Token *generate_operation_nodes(Token *current_token, Node *current_node){
  Node *oper_node = init_node(NULL, current_token->value, current_token->type);
  current_node->left->left = oper_node;
  current_node = oper_node;
  current_token--;
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
  current_token++;
  current_token++;
  while(current_token->type == INT || current_token->type == IDENTIFIER || is_operator(current_token->type)){
    if(current_token->type == INT || current_token->type == IDENTIFIER){
      if((current_token->type != INT && current_token->type != IDENTIFIER) || current_token == NULL){
        printf("Syntax Error hERE\n");
        exit(1);
      }
      current_token++;
      if(!is_operator(current_token->type)){
        current_token--;
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
      current_token--;
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
      current_token++; 
    }
    current_token++;
  }
  return current_token;
}

Node *handle_exit_syscall(Token *current_token, Node *current){
    Node *exit_node = init_node(NULL, current_token->value, EXIT);
    current->right = exit_node;
    current = exit_node;
    current_token++;

    if(current_token->type == END_OF_TOKENS){
      print_error("Invalid Syntax on OPEN", current_token->line_num);
    }
    if(current_token->type == OPEN_PAREN){
      Node *open_paren_node = init_node(NULL, current_token->value, OPEN_PAREN);
      current->left = open_paren_node;
      current_token++;
      if(current_token->type == END_OF_TOKENS){
        print_error("Invalid Syntax on INT", current_token->line_num);
      }
      if(current_token->type == INT || current_token->type == IDENTIFIER){
        current_token++;
        if(is_operator(current_token->type) && current_token != NULL){
          current_token = generate_operation_nodes(current_token, current);
          current_token--;
        } else {
          current_token--;
          Node *expr_node = init_node(NULL, current_token->value, current_token->type);
          current->left->left = expr_node;
        }
        current_token++;
        if(current_token->type == END_OF_TOKENS){
          print_error("Invalid Syntax on cLOSE", current_token->line_num);
        }
        if(current_token->type == CLOSE_PAREN && current_token->type != END_OF_TOKENS){
          Node *close_paren_node = init_node(NULL, current_token->value, CLOSE_PAREN);
          current->left->right = close_paren_node;
          current_token++;
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
  return current;
}

void handle_token_errors(char *error_text, Token *current_token, bool isType){
  if(current_token->type == END_OF_TOKENS || !isType){
    print_error(error_text, current_token->line_num);
  }
}

Node *create_variable_reusage(Token *current_token, Node *current){
  Node *main_identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
  current->left = main_identifier_node;
  current = main_identifier_node;
  current_token++;

  handle_token_errors("Invalid syntax after idenfitier", current_token, is_operator(current_token->type));

  if(is_operator(current_token->type)){
    if(current_token->type == ASSIGNMENT){
      print_error("Invalid Variable Syntax on =", current_token->line_num);
    }
    Node *equals_node = init_node(NULL, current_token->value, current_token->type);
    current->left = equals_node;
    current = equals_node;
    current_token++;
  }
  if(current_token->type == END_OF_TOKENS){
    print_error("Invalid Syntax After Equals", current_token->line_num);
  }

  current_token++;
  if(is_operator(current_token->type)){
    Node *oper_node = init_node(NULL, current_token->value, current_token->type);
    current->left = oper_node;
    current = oper_node;
    current_token--;
    if(current_token->type == INT){
      Node *expr_node = init_node(NULL, current_token->value, INT);
      oper_node->left = expr_node;
      current_token++;
      current_token++;
    } else if(current_token->type == IDENTIFIER){
      Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
      oper_node->left = identifier_node;
      current_token++;
      current_token++;
    } else {
      print_error("ERROR: Expected IDENTIFIER or INT", current_token->line_num);
    }
    current_token++;

    if(is_operator(current_token->type)){
      Node *oper_node = init_node(NULL, current_token->value, current_token->type);
      current->right = oper_node;
      current = oper_node;
      int operation = 1;
      current_token--;
      current_token--;
      while(operation){
        current_token++;
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
        current_token++;
        if(is_operator(current_token->type)){
          current_token++;
          current_token++;
          if(!is_operator(current_token->type)){
            current_token--;
            if(current_token->type == INT){
              Node *expr_node = init_node(NULL, current_token->value, INT);
              current->right = expr_node;
              current_token++;
            } else if(current_token->type == IDENTIFIER){
              Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
              current->right = identifier_node;
              current_token++;
            } else {
              printf("ERROR: UNRECOGNIZED TOKEN!\n");
              exit(1);
            }
            operation = 0;
          } else {
            current_token--;
            current_token--;
            Node *oper_node = init_node(NULL, current_token->value, current_token->type);
            current->right = oper_node;
            current = oper_node;
          }
        } else {
          operation = 0;
        }
      }
    } else {
      current_token--;
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
      current_token++;
    }
  } else {
    current_token--;
    if(current_token->type == INT){
      Node *expr_node = init_node(NULL, current_token->value, INT);
      current->left = expr_node;
      current_token++;
    } else if(current_token->type == IDENTIFIER){
      Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
      current->left = identifier_node;
      current_token++;
    } else if(current_token->type == STRING){
      Node *string_node = init_node(NULL, current_token->value, STRING);
      current->left = string_node;
      current_token++;
    }
  }
  handle_token_errors("Invalid Syntax After Expression", current_token, is_separator(current_token->type));

  current = main_identifier_node;
  if(current_token->type == SEMICOLON){
    Node *semi_node = init_node(NULL, current_token->value, SEMICOLON);
    current->right = semi_node;
    current = semi_node;
  }
  return current;
}

Node *create_variables(Token *current_token, Node *current){
  Node *var_node = init_node(NULL, current_token->value, current_token->type);
  current->left = var_node;
  current = var_node;
  current_token++;
  handle_token_errors("Invalid syntax after INT", current_token, current_token->type == IDENTIFIER);
  if(current_token->type == IDENTIFIER){
    Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
    current->left = identifier_node;
    current = identifier_node;
    current_token++;
  }
  handle_token_errors("Invalid Syntax After Identifier", current_token, is_operator(current_token->type));

  if(is_operator(current_token->type)){
    if(current_token->type != ASSIGNMENT){
      print_error("Invalid Variable Syntax on =", current_token->line_num);
    }
    Node *equals_node = init_node(NULL, current_token->value, current_token->type);
    current->left = equals_node;
    current = equals_node;
    current_token++;
  }
  if(current_token->type == END_OF_TOKENS){
    print_error("Invalid Syntax After Equals", current_token->line_num);
  }

  current_token++;
  if(is_operator(current_token->type)){
    Node *oper_node = init_node(NULL, current_token->value, current_token->type);
    current->left = oper_node;
    current = oper_node;
    current_token--;
    if(current_token->type == INT){
      Node *expr_node = init_node(NULL, current_token->value, INT);
      oper_node->left = expr_node;
      current_token++;
      current_token++;
    } else if(current_token->type == IDENTIFIER){
      Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
      oper_node->left = identifier_node;
      current_token++;
      current_token++;
    } else {
      print_error("ERROR: Expected IDENTIFIER or INT", current_token->line_num);
    }
    current_token++;

    if(is_operator(current_token->type)){
      Node *oper_node = init_node(NULL, current_token->value, current_token->type);
      current->right = oper_node;
      current = oper_node;
      int operation = 1;
      current_token--;
      current_token--;
      while(operation){
        current_token++;
        if(current_token->type == INT){
          Node *expr_node = init_node(NULL, current_token->value, INT);
          current->left = expr_node;
        } else if(current_token->type == IDENTIFIER){
          Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
          current->left = identifier_node;
        } else {
          printf("ERROR: Unexpected Token\n");
          exit(1);
        }
        current_token++;
        if(is_operator(current_token->type)){
          current_token++;
          current_token++;
          if(!is_operator(current_token->type)){
            current_token--;
            if(current_token->type == INT){
              Node *expr_node = init_node(NULL, current_token->value, INT);
              current->right = expr_node;
              current_token++;
            } else if(current_token->type == IDENTIFIER){
              Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
              current->right = identifier_node;
              current_token++;
            } else {
              printf("ERROR: UNRECOGNIZED TOKEN!\n");
              exit(1);
            }
            operation = 0;
          } else {
            current_token--;
            current_token--;
            Node *oper_node = init_node(NULL, current_token->value, current_token->type);
            current->right = oper_node;
            current = oper_node;
          }
        } else {
          operation = 0;
        }
      }
    } else {
      current_token--;
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
      current_token++;
    }
  } else {
    current_token--;
    if(current_token->type == INT){
      Node *expr_node = init_node(NULL, current_token->value, INT);
      current->left = expr_node;
      current_token++;
    } else if(current_token->type == IDENTIFIER){
      Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
      current->left = identifier_node;
      current_token++;
    } else if(current_token->type == STRING){
      Node *string_node = init_node(NULL, current_token->value, STRING);
      current->left = string_node;
      current_token++;
    }
  }

  //if(current_token->type == OPERATOR){
  //  current_token = generate_operation_nodes(current_token, current);
  //}

  handle_token_errors("Invalid Syntax After Expression", current_token, is_separator(current_token->type));

  current = var_node;
  if(current_token->type == SEMICOLON){
    Node *semi_node = init_node(NULL, current_token->value, SEMICOLON);
    current->right = semi_node;
    current = semi_node;
  }
  return current;
}

Node *handle_write(Token **current_token, Node *current){
  Token *token = *current_token; // currently at WRITE

  token++; // consume WRITE
  handle_token_errors("Invalid Syntax on OPEN", token, token->type == OPEN_PAREN);

  token++; // consume '('
  handle_token_errors("Invalid Syntax on EXP", token,
                      token->type == INT || token->type == STRING ||
                      token->type == IDENTIFIER || token->type == BOOL);
  Node *expr_node = init_node(NULL, token->value, token->type);

  token++; // move past expression
  while(token->type != CLOSE_PAREN && token->type != END_OF_TOKENS){
    token++;
  }
  handle_token_errors("Invalid Syntax on CLOSE", token, token->type == CLOSE_PAREN);
  token++; // consume ')'

  handle_token_errors("Invalid Syntax on SEMI", token, token->type == SEMICOLON);
  Node *write_node = init_node(NULL, NULL, WRITE);
  write_node->left = expr_node;
  current->right = write_node;
  current = write_node;

  token++; // consume ';'
  *current_token = token - 1; // parser loop will increment after return
  return current;
}

Node *handle_if(Token **current_token, Node *current){
  Token *token = *current_token;

  Node *cond_node = init_node(NULL, token->value, token->type);
  current->right = cond_node;
  current = cond_node;
  token++;

  if(cond_node->type == ELSE){
    // For `else` blocks, no parentheses follow. Reset the caller's token
    // pointer to the `else` token so that the parser's main loop will
    // advance to the upcoming `{` and process the block normally.
    *current_token = token - 1;
    return current;
  }

  handle_token_errors("Invalid Syntax on OPEN", token, token->type == OPEN_PAREN);
  Node *open_paren_node = init_node(NULL, token->value, OPEN_PAREN);
  current->left = open_paren_node;
  token++;

  handle_token_errors("Invalid Syntax on EXP", token,
                      token->type == INT ||
                      token->type == STRING ||
                      token->type == IDENTIFIER ||
                      token->type == BOOL);
  Node *expr_node = init_node(NULL, token->value, token->type);
  open_paren_node->left = expr_node;
  token++;

  while(token->type != CLOSE_PAREN && token->type != END_OF_TOKENS){
    token++;
  }

  handle_token_errors("Invalid Syntax on CLOSE", token, token->type == CLOSE_PAREN);
  Node *close_paren_node = init_node(NULL, token->value, CLOSE_PAREN);
  open_paren_node->right = close_paren_node;
  current = close_paren_node;

  *current_token = token;
  return current;
}

Node *handle_while(Token **current_token, Node *current){
  return handle_if(current_token, current);
}

Node *handle_for(Token **current_token, Node *current){
  Token *token = *current_token;

  Node *for_node = init_node(NULL, token->value, FOR);
  current->right = for_node;
  current = for_node;
  token++;

  handle_token_errors("Invalid Syntax on OPEN", token, token->type == OPEN_PAREN);
  Node *open_paren_node = init_node(NULL, token->value, OPEN_PAREN);
  current->left = open_paren_node;
  token++;

  // simple parsing: init expression until first semicolon
  while(token->type != SEMICOLON && token->type != END_OF_TOKENS){
    token++;
  }
  handle_token_errors("Invalid Syntax on SEMI", token, token->type == SEMICOLON);
  Node *first_semi = init_node(NULL, token->value, SEMICOLON);
  open_paren_node->left = first_semi;
  token++;

  // condition expression until next semicolon
  while(token->type != SEMICOLON && token->type != END_OF_TOKENS){
    token++;
  }
  handle_token_errors("Invalid Syntax on SEMI", token, token->type == SEMICOLON);
  Node *second_semi = init_node(NULL, token->value, SEMICOLON);
  first_semi->right = second_semi;
  token++;

  // update expression until close paren
  while(token->type != CLOSE_PAREN && token->type != END_OF_TOKENS){
    token++;
  }
  handle_token_errors("Invalid Syntax on CLOSE", token, token->type == CLOSE_PAREN);
  Node *close_paren_node = init_node(NULL, token->value, CLOSE_PAREN);
  second_semi->right = close_paren_node;
  current = close_paren_node;

  *current_token = token;
  return current;
}

Node *handle_fn(Token *current_token, Node *current){
  Node *fn_node = init_node(NULL, current_token->value, FN);
  current->right = fn_node;
  current = fn_node;
  current_token++;

  handle_token_errors("Invalid Syntax on IDENT", current_token, current_token->type == IDENTIFIER);
  Node *identifier_node = init_node(NULL, current_token->value, IDENTIFIER);
  current->left = identifier_node;
  current_token++;

  handle_token_errors("Invalid Syntax on OPEN", current_token, current_token->type == OPEN_PAREN);
  Node *open_paren_node = init_node(NULL, current_token->value, OPEN_PAREN);
  identifier_node->left = open_paren_node;
  current_token++;

  handle_token_errors("Invalid Syntax on CLOSE", current_token, current_token->type == CLOSE_PAREN);
  Node *close_paren_node = init_node(NULL, current_token->value, CLOSE_PAREN);
  open_paren_node->right = close_paren_node;
  current = close_paren_node;
  return current;
}
Node *parser(Token *tokens){
  Token *current_token = &tokens[0];
  Node *root = init_node(NULL, NULL, BEGINNING);

  Node *current = root;

  Node *open_curly = NULL;

  curly_stack *stack = malloc(sizeof(curly_stack));
  stack->top = -1;
  if (!push_curly(stack, root)) { // root acts as the initial block
    printf("ERROR: curly stack overflow\n");
    exit(1);
  }

  bool allow_else = false;
  bool after_if_block = false;

  while(current_token->type != END_OF_TOKENS){
    if(current == NULL){
      break;
    }
    switch(current_token->type){
      case LET:
      case FN:
      case FOR:
      case WHILE:
      case WRITE:
      case EXIT: {
        if(allow_else && after_if_block){
          allow_else = false;
          after_if_block = false;
        }
        if(current_token->type == LET){
          current = create_variables(current_token, current);
        } else if(current_token->type == FN){
          current = handle_fn(current_token, current);
        } else if(current_token->type == FOR){
          current = handle_for(&current_token, current);
        } else if(current_token->type == WHILE){
          current = handle_while(&current_token, current);
        } else if(current_token->type == WRITE){
          current = handle_write(&current_token, current);
        } else if(current_token->type == EXIT){
          current = handle_exit_syscall(current_token, current);
        }
        // statement added as right child; no additional bookkeeping needed
        break;
      }
      case IF:
        current = handle_if(&current_token, current);
        allow_else = true;
        after_if_block = false;
        break;
      case ELSE_IF:
        if(!(allow_else && after_if_block)){
          print_error("Unexpected else without matching if", current_token->line_num);
        }
        current = handle_if(&current_token, current);
        allow_else = true;
        after_if_block = false;
        break;
      case ELSE:
        if(!(allow_else && after_if_block)){
          print_error("Unexpected else without matching if", current_token->line_num);
        }
        current = handle_if(&current_token, current);
        allow_else = false;
        after_if_block = false;
        break;
      case OPEN_CURLY:
      case CLOSE_CURLY:
      case OPEN_BRACKET:
      case CLOSE_BRACKET:
      case OPEN_PAREN:
      case CLOSE_PAREN:
      case SEMICOLON:
      case COMMA:
        // structural tokens do not reset allow_else
        if(current_token->type == OPEN_CURLY){
          open_curly = init_node(NULL, current_token->value, current_token->type);
          current->left = open_curly;
          if (!push_curly(stack, open_curly)) {
            printf("ERROR: curly stack overflow\n");
            exit(1);
          }
          current = open_curly;
        }
        if(current_token->type == CLOSE_CURLY){
          Node *close_curly = init_node(NULL, current_token->value, current_token->type);
          open_curly = pop_curly(stack);
          if(open_curly == NULL){
            printf("ERROR: Expected Open Parenthesis!\n");
            exit(1);
          }
          current->right = close_curly;
          current = close_curly;
          if(allow_else){
            after_if_block = true;
          }
        }
        break;
      case ASSIGNMENT:
      case NOT:
      case PLUS_PLUS:
      case MINUS_MINUS:
      case PLUS_EQUALS:
      case MINUS_EQUALS:
      case PLUS:
      case DASH:
      case SLASH:
      case STAR:
      case PERCENT:
        break;
      case INT:
        break;
      case IDENTIFIER:
        if(allow_else && after_if_block){
          allow_else = false;
          after_if_block = false;
        }
        current_token--;
        if(current_token->type == SEMICOLON || current_token->type == OPEN_CURLY || current_token->type == CLOSE_CURLY){
          current_token++;
          current = create_variable_reusage(current_token, current);
        } else {
          current_token++;
        }
        break;
      case STRING:
        if(allow_else && after_if_block){
          allow_else = false;
          after_if_block = false;
        }
        break;
      case BOOL:
        if(allow_else && after_if_block){
          allow_else = false;
          after_if_block = false;
        }
        break;
      case OR:
      case AND:
      case LESS:
      case LESS_EQUALS:
      case GREATER:
      case GREATER_EQUALS:
      case EQUALS:
      case NOT_EQUALS:
        if(allow_else && after_if_block){
          allow_else = false;
          after_if_block = false;
        }
        break;
      case BEGINNING:
        if(allow_else && after_if_block){
          allow_else = false;
          after_if_block = false;
        }
        break;
      case END_OF_TOKENS:
        break;
    }
    current_token++;
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
