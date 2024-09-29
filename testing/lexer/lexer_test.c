#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

size_t line_number = 0;

void print_token(Token token){
  printf("TOKEN VALUE: ");
  printf("'");
  for(int i = 0; token.value[i] != '\0'; i++){
    printf("%c", token.value[i]);
  }
  printf("'");
  printf("\nline number: %zu", token.line_num);

  switch(token.type){
    case BEGINNING:
      printf("BEGINNING\n");
      break;   
    case INT:
      printf(" TOKEN TYPE: INT\n");
      break;
    case STRING:
      printf(" TOKEN TYPE: STRING\n");
      break;
    case IDENTIFIER:
      printf(" TOKEN TYPE: IDENTIFIER\n");
      break;
    case OPEN_BRACKET:
      printf(" TOKEN TYPE: OPEN_BRACKET\n");
      break;
    case CLOSE_BRACKET:
      printf(" TOKEN TYPE: CLOSE_BRACKET\n");
      break;
    case OPEN_CURLY:
      printf(" TOKEN TYPE: OPEN_CURLY\n");
      break;
    case CLOSE_CURLY:
      printf(" TOKEN TYPE: CLOSE_CURLY\n");
      break;
    case OPEN_PAREN:
      printf(" TOKEN TYPE: OPEN_PAREN\n");
      break;
    case CLOSE_PAREN:
      printf(" TOKEN TYPE: CLOSE_PAREN\n");
      break;
    case ASSIGNMENT:
      printf(" TOKEN TYPE: ASSIGNMENT\n");
      break;
    case EQUALS:
      printf(" TOKEN TYPE: EQUALS\n");
      break;
    case NOT_EQUALS:
      printf(" TOKEN TYPE: NOT_EQUALS\n");
      break;
    case NOT:
      printf(" TOKEN TYPE: NOT\n");
      break;
    case LESS:
      printf(" TOKEN TYPE: LESS\n");
      break;
    case LESS_EQUALS:
      printf(" TOKEN TYPE: LESS_EQUALS\n");
      break;
    case GREATER:      
      printf(" TOKEN TYPE: GREATER\n");
      break;
    case GREATER_EQUALS:
      printf(" TOKEN TYPE: GREATER_EQUALS\n");
      break;
    case OR:
      printf(" TOKEN TYPE: OR\n");
      break;
    case AND:
      printf(" TOKEN TYPE: AND\n");
      break;
    case SEMICOLON:
      printf(" TOKEN TYPE: SEMICOLON\n");
      break;
    case COMMA:
      printf(" TOKEN TYPE: COMMA\n");
      break;
    case PLUS_PLUS:
      printf(" TOKEN TYPE: PLUS_PLUS\n");
      break;
    case MINUS_MINUS:
      printf(" TOKEN TYPE: MINUS_MINUS\n");
      break;
    case PLUS_EQUALS:
      printf(" TOKEN TYPE: PLUS_EQUALS\n");
      break;
    case MINUS_EQUALS:
      printf(" TOKEN TYPE: MINUS_EQUALS\n");
      break;
    case PLUS:
      printf(" TOKEN TYPE: PLUS\n");
      break;
    case DASH:
      printf(" TOKEN TYPE: DASH\n");
      break;
    case SLASH:
      printf(" TOKEN TYPE: SLASH\n");
      break;
    case STAR:
      printf(" TOKEN TYPE: STAR\n");
      break;
    case PERCENT:
      printf(" TOKEN TYPE: PERCENT\n");
      break;
    case LET:
      printf(" TOKEN TYPE: LET\n");
      break;
    case FN:
      printf(" TOKEN TYPE: FN\n");
      break;
    case IF:
      printf(" TOKEN TYPE: IF\n");
      break;
    case ELSE_IF:
      printf(" TOKEN TYPE: ELSE_IF\n");
      break;
    case ELSE:
      printf(" TOKEN TYPE: ELSE\n");
      break;      
    case FOR:
      printf(" TOKEN TYPE: FOR\n");
      break;
    case FOR_EACH:
      printf(" TOKEN TYPE: FOR_EACH\n");
      break;
    case WHILE:
      printf(" TOKEN TYPE: WHILE\n");
      break;
    case WRITE:
      printf(" TOKEN TYPE: WRITE\n");
      break;
    case EXIT:
      printf(" TOKEN TYPE: EXIT\n");
      break;
    case END_OF_TOKENS:
      printf(" END OF TOKENS\n");
      break;
  }
}

Token *generate_number(char *current, int *current_index){
  Token *token = malloc(sizeof(Token));
  token->line_num = malloc(sizeof(size_t));
  token->line_num = line_number;
  token->type = INT;
  char *value = malloc(sizeof(char) * 8);
  int value_index = 0;
  while(isdigit(current[*current_index]) && current[*current_index] != '\0'){
    if(!isdigit(current[*current_index])){
      break;
    }
    value[value_index] = current[*current_index];
    value_index++;
    *current_index += 1;
  }
  value[value_index] = '\0';
  token->value = value;
  return token;
}

Token *generate_keyword_or_identifier(char *current, int *current_index){
  Token *token = malloc(sizeof(Token));
  token->line_num = malloc(sizeof(size_t));
  token->line_num = line_number;
  char *keyword = malloc(sizeof(char) * 8);
  int keyword_index = 0;
  while(isalpha(current[*current_index]) && current[*current_index] != '\0'){
    keyword[keyword_index] = current[*current_index];
    keyword_index++;
    *current_index += 1;
  }
  keyword[keyword_index] = '\0';
  if(strcmp(keyword, "exit") == 0){
    token->type = EXIT;
    token->value = "EXIT";
  } else if(strcmp(keyword, "let") == 0){
    token->type = LET;
    token->value = "let";
  } else if(strcmp(keyword, "if") == 0){
    token->type = IF;
    token->value = "if";
  } else if(strcmp(keyword, "while") == 0){
    token->type = WHILE;
    token->value = "while";
  } else if(strcmp(keyword, "write") == 0){
    token->type = WRITE;
    token->value = "write";
  } else if(strcmp(keyword, "eq") == 0){
    token->type = EQUALS;
    token->value = "eq";
  } else if(strcmp(keyword, "neq") == 0){
    token->type = NOT_EQUALS;
    token->value = "neq";
  } else if(strcmp(keyword, "less") == 0){
    token->type = LESS;
    token->value = "less";
  } else if(strcmp(keyword, "elif") == 0){
    token->type = ELSE_IF;
    token->value = "elif";
  } else if(strcmp(keyword, "for") == 0){
    token->type = FOR;
    token->value = "for";
  } else if(strcmp(keyword, "foreach") == 0){
    token->type = FOR_EACH;
    token->value = "foreach";
  } else if(strcmp(keyword, "fn") == 0){ 
    token->type = FN;
    token->value = "fn";
  } else if(strcmp(keyword, "else") == 0){ 
    token->type = ELSE;
    token->value = "else";
  } else if(strcmp(keyword, "or") == 0){ 
    token->type = OR;
    token->value = "or";
  } else if(strcmp(keyword, "and") == 0){ 
    token->type = AND;
    token->value = "and";
  } else {
    token->type = IDENTIFIER;
    token->value = keyword;
  }
  return token;
}

Token *generate_string_token(char *current, int *current_index){
  Token *token = malloc(sizeof(Token));
  token->line_num = malloc(sizeof(size_t));
  token->line_num = line_number;
  char *value = malloc(sizeof(char) * 64);
  int value_index = 0;
  *current_index += 1;
  while(current[*current_index] != '"'){
    value[value_index] = current[*current_index];
    value_index++;
    *current_index += 1;
  }
  value[value_index] = '\0';
  token->type = STRING;
  token->value = value;
  return token;
}

Token *generate_separator_or_operator(char *current, int *current_index, TokenType type){
  Token *token = malloc(sizeof(Token));
  token->value = malloc(sizeof(char) * 2);
  token->value[0] = current[*current_index];
  token->value[1] = '\0';
  token->line_num = malloc(sizeof(size_t));
  token->line_num = line_number;
  token->type = type;
  return token;
}

Token *generate_two_char_operator(char *current, int *current_index, TokenType type) {
    Token *token = malloc(sizeof(Token));
    token->value = malloc(sizeof(char) * 3);
    token->value[0] = current[*current_index];
    token->value[1] = current[*current_index + 1];
    token->value[2] = '\0';
    *current_index += 2;
    token->line_num = malloc(sizeof(size_t));
    token->line_num = line_number;
    token->type = type;
    return token;
}

size_t tokens_index;

Token *lexer(FILE *file){
  int length;
  char *current = 0;

  fseek(file, 0, SEEK_END);
  length = ftell(file);
  fseek(file, 0, SEEK_SET);

  current = malloc(sizeof(char) * length);
  fread(current, 1, length, file);

  fclose(file);

  current[length] = '\0';
  int current_index = 0;

  int number_of_tokens = 12;
  int tokens_size = 0;
  Token *tokens = malloc(sizeof(Token) * number_of_tokens);
  tokens_index = 0;

  while(current[current_index] != '\0'){
    Token *token = malloc(sizeof(Token));
    tokens_size++;
    if(tokens_size > number_of_tokens){
      number_of_tokens *= 1.5;
      tokens = realloc(tokens, sizeof(Token) * number_of_tokens);
    }
    if (current[current_index] == '=' && current[current_index + 1] == '='){
      token = generate_two_char_operator(current, &current_index, EQUALS);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if (current[current_index] == '!' && current[current_index + 1] == '=') {
      token = generate_two_char_operator(current, &current_index, NOT_EQUALS);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if (current[current_index] == '<' && current[current_index + 1] == '='){
      token = generate_two_char_operator(current, &current_index, LESS_EQUALS); 
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if (current[current_index] == '>' && current[current_index + 1] == '='){
      token = generate_two_char_operator(current, &current_index, GREATER_EQUALS);  
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if (current[current_index] == '&' && current[current_index + 1] == '&'){
      token = generate_two_char_operator(current, &current_index, AND);  
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if (current[current_index] == '|' && current[current_index + 1] == '|'){
      token = generate_two_char_operator(current, &current_index, OR);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if (current[current_index] == '+' && current[current_index + 1] == '+'){
      token = generate_two_char_operator(current, &current_index, PLUS_PLUS);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if (current[current_index] == '-' && current[current_index + 1] == '-'){
      token = generate_two_char_operator(current, &current_index, MINUS_MINUS);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if (current[current_index] == '+' && current[current_index + 1] == '='){
      token = generate_two_char_operator(current, &current_index, PLUS_EQUALS);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if (current[current_index] == '-' && current[current_index + 1] == '='){
      token = generate_two_char_operator(current, &current_index, MINUS_EQUALS);
      tokens[tokens_index] = *token;
      tokens_index++; 
    } else if(current[current_index] == ';'){
      token = generate_separator_or_operator(current, &current_index, SEMICOLON);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == ','){
      token = generate_separator_or_operator(current, &current_index, COMMA);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '('){
      token = generate_separator_or_operator(current, &current_index, OPEN_PAREN);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == ')'){
      token = generate_separator_or_operator(current, &current_index, CLOSE_PAREN);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '{'){
      token = generate_separator_or_operator(current, &current_index, OPEN_CURLY);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '}'){
      token = generate_separator_or_operator(current, &current_index, CLOSE_CURLY);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '='){
      token = generate_separator_or_operator(current, &current_index, ASSIGNMENT);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '+'){
      token = generate_separator_or_operator(current, &current_index, PLUS);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '-'){
      token = generate_separator_or_operator(current, &current_index, DASH);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '*'){
      token = generate_separator_or_operator(current, &current_index, STAR);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '/'){
      token = generate_separator_or_operator(current, &current_index, SLASH);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '%'){
      token = generate_separator_or_operator(current, &current_index, PERCENT);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '>'){
      token = generate_separator_or_operator(current, &current_index, GREATER);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '<'){
      token = generate_separator_or_operator(current, &current_index, LESS);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '['){
      token = generate_separator_or_operator(current, &current_index, OPEN_BRACKET);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == ']'){
      token = generate_separator_or_operator(current, &current_index, CLOSE_BRACKET);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '!'){
      token = generate_separator_or_operator(current, &current_index, NOT);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(current[current_index] == '"'){
      token = generate_string_token(current, &current_index);
      tokens[tokens_index] = *token;
      tokens_index++;
    } else if(isdigit(current[current_index])){
      token = generate_number(current, &current_index); 
      tokens[tokens_index] = *token;
      tokens_index++;
      current_index--;
    } else if(isalpha(current[current_index])){
      token = generate_keyword_or_identifier(current, &current_index);
      tokens[tokens_index] = *token;
      tokens_index++;
      current_index--;
    } else if(current[current_index] == '\n'){
      line_number += 1;
    } 
    free(token);
    current_index++;
  }
  tokens[tokens_index].value = '\0';
  tokens[tokens_index].type = END_OF_TOKENS;
  return tokens;
}

int main() {
  FILE *file;
  file = fopen("test3.hs", "r");

  Token *tokens = lexer(file);
  
  int counter = 0;
  while(tokens[counter].type != END_OF_TOKENS){
    print_token(tokens[counter]);
    counter++;
  }
}
