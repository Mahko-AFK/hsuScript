#ifndef TOKEN_HELPERS_H
#define TOKEN_HELPERS_H

#include "lexer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static inline Token *peek(Token **pp) {
  return *pp;
}

static inline Token *next(Token **pp) {
  (*pp)++;
  return *pp;
}

static inline Token *prev(Token **pp) {
  (*pp)--;
  return *pp;
}

static inline bool match(Token **pp, TokenType type) {
  if ((*pp)->type != type)
    return false;
  (*pp)++;
  return true;
}

static inline Token *expect(Token **pp, TokenType type, const char *msg) {
  if ((*pp)->type != type) {
    printf("ERROR: %s on line number: %zu\n", msg, (*pp)->line_num);
    exit(1);
  }
  Token *t = *pp;
  (*pp)++;
  return t;
}

#endif
