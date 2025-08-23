#include "tools.h"
#include <stdbool.h>

bool is_operator(TokenType token) {
    switch (token) {
        case ASSIGNMENT:
        case EQUALS:
        case NOT_EQUALS:
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
            return true;
        default:
            return false;
    }
}

bool is_comparator(TokenType token) {
    switch (token) {
        case EQUALS:
        case NOT_EQUALS:
        case LESS:
        case LESS_EQUALS:
        case GREATER:
        case GREATER_EQUALS:
            return true;
        default:
            return false;
    }
}

bool is_separator(TokenType token) {
    switch (token) {
        case SEMICOLON:
        case COMMA:
        case OPEN_BRACKET:
        case CLOSE_BRACKET:
        case OPEN_CURLY:
        case CLOSE_CURLY:
        case OPEN_PAREN:
        case CLOSE_PAREN:
            return true;
        default:
            return false;
    }
}

bool is_keyword(TokenType token) {
    switch (token) {
        case LET:
        case FN:
        case IF:
        case ELSE_IF:
        case ELSE:
        case FOR:
        case WHILE:
        case WRITE:
        case EXIT:
            return true;
        default:
            return false;
    }
}

bool is_literal(TokenType token) {
    switch (token) {
        case INT:
        case STRING:
        case IDENTIFIER:
            return true;
        default:
            return false;
    }
}
