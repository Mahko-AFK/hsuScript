#ifndef TOOLS_H
#define TOOLS_H

#include <stdbool.h>
#include "lexer.h"

bool is_operator(TokenType token);
bool is_comparator(TokenType token);
bool is_separator(TokenType token);
bool is_keyword(TokenType token);
bool is_literal(TokenType token);

#endif
