#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "lexer.h"
#include "tools.h"

typedef struct Node {
    char *value;
    TokenType type;
    struct Node *left;
    struct Node *right;
} Node;

static Token *tokens;
static size_t current;

static Token *peek(void) { return &tokens[current]; }
static Token *previous(void) { return &tokens[current-1]; }
static bool check(TokenType type) { return peek()->type == type; }
static Token *advance_token(void) {
    if(peek()->type != END_OF_TOKENS) current++;
    return previous();
}
static bool match(TokenType type) {
    if(check(type)) { advance_token(); return true; }
    return false;
}

static void error(const char *msg, size_t line){
    printf("ERROR: %s on line number: %zu\n", msg, line);
    exit(1);
}

static Node *new_node(Token *t){
    Node *n = malloc(sizeof(Node));
    n->value = t->value;
    n->type = t->type;
    n->left = n->right = NULL;
    return n;
}

static Node *parse_expression(void);
static Node *parse_statement(void);
static Node *parse_block(void);

static void expect(TokenType type, const char *msg){
    if(!match(type)) error(msg, peek()->line_num);
}

/* Expression parsing with precedence */

static Node *parse_primary(void){
    if(match(IDENTIFIER) || match(INT) || match(STRING)){
        return new_node(previous());
    }
    if(match(OPEN_PAREN)){
        Node *expr = parse_expression();
        expect(CLOSE_PAREN, "Invalid Syntax on CLOSE");
        return expr;
    }
    error("Invalid Expression", peek()->line_num);
    return NULL;
}

static Node *parse_postfix(void){
    Node *node = parse_primary();
    while(match(PLUS_PLUS) || match(MINUS_MINUS)){
        Node *op = new_node(previous());
        op->left = node;
        node = op;
    }
    return node;
}

static Node *parse_factor(void){
    Node *node = parse_postfix();
    while(match(STAR) || match(SLASH) || match(PERCENT)){
        Node *op = new_node(previous());
        op->left = node;
        op->right = parse_postfix();
        node = op;
    }
    return node;
}

static Node *parse_term(void){
    Node *node = parse_factor();
    while(match(PLUS) || match(DASH)){
        Node *op = new_node(previous());
        op->left = node;
        op->right = parse_factor();
        node = op;
    }
    return node;
}

static Node *parse_comparison(void){
    Node *node = parse_term();
    while(match(LESS) || match(LESS_EQUALS) || match(GREATER) || match(GREATER_EQUALS)){
        Node *op = new_node(previous());
        op->left = node;
        op->right = parse_term();
        node = op;
    }
    return node;
}

static Node *parse_equality(void){
    Node *node = parse_comparison();
    while(match(EQUALS) || match(NOT_EQUALS)){
        Node *op = new_node(previous());
        op->left = node;
        op->right = parse_comparison();
        node = op;
    }
    return node;
}

static Node *parse_logic_and(void){
    Node *node = parse_equality();
    while(match(AND)){
        Node *op = new_node(previous());
        op->left = node;
        op->right = parse_equality();
        node = op;
    }
    return node;
}

static Node *parse_expression(void){
    Node *node = parse_logic_and();
    while(match(OR)){
        Node *op = new_node(previous());
        op->left = node;
        op->right = parse_logic_and();
        node = op;
    }
    return node;
}

/* Statement parsing */

static Node *parse_let(void){
    Token *let_tok = advance_token();
    Node *let_node = new_node(let_tok);
    expect(IDENTIFIER, "Invalid Syntax After LET");
    Node *id_node = new_node(previous());
    expect(ASSIGNMENT, "Invalid Variable Syntax on =");
    Node *expr = parse_expression();
    expect(SEMICOLON, "Invalid Syntax on SEMI");
    let_node->left = id_node;
    id_node->left = expr;
    return let_node;
}

static Node *parse_write(void){
    Token *w_tok = advance_token();
    Node *w_node = new_node(w_tok);
    expect(OPEN_PAREN, "Invalid Syntax on OPEN");
    Node *expr = parse_expression();
    expect(CLOSE_PAREN, "Invalid Syntax on CLOSE");
    expect(SEMICOLON, "Invalid Syntax on SEMI");
    w_node->left = expr;
    return w_node;
}

static Node *parse_exit(void){
    Token *e_tok = advance_token();
    Node *e_node = new_node(e_tok);
    expect(OPEN_PAREN, "Invalid Syntax on OPEN");
    Node *expr = parse_expression();
    expect(CLOSE_PAREN, "Invalid Syntax on CLOSE");
    expect(SEMICOLON, "Invalid Syntax on SEMI");
    e_node->left = expr;
    return e_node;
}

static Node *parse_block(void){
    expect(OPEN_CURLY, "Invalid Syntax on OPEN_CURLY");
    Node *block = new_node(previous());
    Node **current_ptr = &block->left;
    while(!check(CLOSE_CURLY) && peek()->type != END_OF_TOKENS){
        Node *stmt = parse_statement();
        *current_ptr = stmt;
        current_ptr = &stmt->right;
    }
    expect(CLOSE_CURLY, "Invalid Syntax on CLOSE_CURLY");
    return block;
}

static Node *parse_if(void){
    Token *if_tok = advance_token();
    Node *if_node = new_node(if_tok);
    expect(OPEN_PAREN, "Invalid Syntax on OPEN");
    Node *cond = parse_expression();
    expect(CLOSE_PAREN, "Invalid Syntax on CLOSE");
    Node *body = parse_block();
    if_node->left = cond;
    cond->right = body;

    Node *current_if = if_node;
    while(match(ELSE_IF)){
        Node *elif_node = new_node(previous());
        expect(OPEN_PAREN, "Invalid Syntax on OPEN");
        Node *elif_cond = parse_expression();
        expect(CLOSE_PAREN, "Invalid Syntax on CLOSE");
        Node *elif_body = parse_block();
        current_if->right = elif_node;
        elif_node->left = elif_cond;
        elif_cond->right = elif_body;
        current_if = elif_node;
    }
    if(match(ELSE)){
        Node *else_node = new_node(previous());
        Node *else_body = parse_block();
        current_if->right = else_node;
        else_node->left = else_body;
    }
    return if_node;
}

static Node *parse_for(void){
    Token *for_tok = advance_token();
    Node *for_node = new_node(for_tok);
    expect(OPEN_PAREN, "Invalid Syntax on OPEN");
    Node *init = parse_let();
    Node *cond = parse_expression();
    expect(SEMICOLON, "Invalid Syntax on SEMI");
    Node *post = parse_expression();
    expect(CLOSE_PAREN, "Invalid Syntax on CLOSE");
    Node *body = parse_block();
    for_node->left = init;
    init->right = cond;
    cond->right = post;
    post->right = body;
    return for_node;
}

static Node *parse_fn(void){
    Token *fn_tok = advance_token();
    Node *fn_node = new_node(fn_tok);
    expect(IDENTIFIER, "Invalid Syntax after FN");
    Node *id_node = new_node(previous());
    expect(OPEN_PAREN, "Invalid Syntax on OPEN");
    expect(CLOSE_PAREN, "Invalid Syntax on CLOSE");
    Node *body = parse_block();
    fn_node->left = id_node;
    id_node->right = body;
    return fn_node;
}

static Node *parse_statement(void){
    switch(peek()->type){
        case LET:
            return parse_let();
        case WRITE:
            return parse_write();
        case EXIT:
            return parse_exit();
        case FOR:
            return parse_for();
        case IF:
            return parse_if();
        case FN:
            return parse_fn();
        default:
            error("Unexpected token", peek()->line_num);
            return NULL;
    }
}

Node *parser(Token *toks){
    tokens = toks;
    current = 0;
    Node *program = malloc(sizeof(Node));
    program->value = "PROGRAM";
    program->type = BEGINNING;
    program->left = program->right = NULL;
    Node **current_ptr = &program->left;
    while(peek()->type != END_OF_TOKENS){
        Node *stmt = parse_statement();
        *current_ptr = stmt;
        current_ptr = &stmt->right;
    }
    return program;
}

void print_tree(Node *node, int indent, char *label, int is_last){
    if(!node) return;
    for(int i=0;i<indent;i++) printf(" ");
    printf("%s -> %s\n", label, node->value ? node->value : "NULL");
    indent += 4;
    if(node->left) print_tree(node->left, indent, "├── left", node->right==NULL);
    if(node->right) print_tree(node->right, indent, "└── right", 1);
}

void free_tree(Node *node){
    if(!node) return;
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}

