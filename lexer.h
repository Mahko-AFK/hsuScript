#ifndef LEXER_H_
#define LEXER_H_

#include <stdio.h>

typedef enum {
  BEGINNING,

  INT,
  STRING,
  IDENTIFIER,

  OPEN_BRACKET,
  CLOSE_BRACKET,
  OPEN_CURLY,
  CLOSE_CURLY,
  OPEN_PAREN,
  CLOSE_PAREN,

  ASSIGNMENT,
  EQUALS,
  NOT_EQUALS,
  NOT,

  LESS,
  LESS_EQUALS,
  GREATER,
  GREATER_EQUALS,

  OR,
  AND,
  
  SEMICOLON,
  COMMA,

  PLUS_PLUS,
  MINUS_MINUS,
  PLUS_EQUALS,
  MINUS_EQUALS,
  
  PLUS,
  DASH,
  SLASH,
  STAR,
  PERCENT,

  //Reserved Keywords
  LET,
  FN,
  IF,
  ELSE_IF,
  ELSE,
  FOR,
  FOR_EACH,
  WHILE,
  WRITE,
  EXIT,

  //End of Pointer Type
  END_OF_TOKENS,
} TokenType;

typedef struct {
  TokenType type;
  char *value;
  size_t line_num;
} Token;

void print_token(Token token);
Token *generate_number(char *current, int *current_index);
Token *generate_keyword(char *current, int *current_index);
Token *generate_separator_or_operator(char *current, int *current_index, TokenType type);
Token *generate_separator_or_operator(char *current, int *current_index, TokenType type);
Token *lexer(FILE *file);
void free_tokens(Token *tokens);

#endif
