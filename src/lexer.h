#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <string.h>

#include "defs.h"
#include "utils.h"

typedef enum lexer_token_type {
    TOKEN_EOF = 0,
    TOKEN_BREAK = 256,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_ELSE,
    TOKEN_FLOAT,
    TOKEN_IF,
    TOKEN_INPUT,
    TOKEN_INT,
    TOKEN_OUTPUT,
    TOKEN_SWITCH,
    TOKEN_WHILE,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_SEMICOLON,
    TOKEN_ASSIGN,
    TOKEN_RELOP_EQ,
    TOKEN_RELOP_NE,
    TOKEN_RELOP_LT,
    TOKEN_RELOP_GT,
    TOKEN_RELOP_GE,
    TOKEN_RELOP_LE,
    TOKEN_ADDOP_PLUS,
    TOKEN_ADDOP_MINUS,
    TOKEN_MULOP_MUL,
    TOKEN_MULOP_DIV,
    TOKEN_OR,
    TOKEN_AND,
    TOKEN_NOT,
    TOKEN_CAST_INT,
    TOKEN_CAST_FLOAT,
    TOKEN_ID,
    TOKEN_NUM_INT,
    TOKEN_NUM_FLOAT
} lexer_token_type;

b8 lexer_begin(FILE *input_file);
void lexer_end(void);
i32 lexer_next(void);
i32 lexer_line(void);
i32 lexer_column(void);
const char *lexer_text(void);
i32 lexer_error_count(void);

static inline void lexer_capture_preview(
    char *line_preview,
    i32 *line_preview_len,
    i32 line_preview_cap,
    const char *text,
    i32 text_len
) {
    i32 i = 0;
    for (i = 0; i < text_len; i++) {
        char c = text[i];
        if (c == '\n') {
            *line_preview_len = 0;
            continue;
        }

        if (*line_preview_len < line_preview_cap - 1) {
            line_preview[(*line_preview_len)++] = c;
            line_preview[*line_preview_len] = '\0';
        }
    }
}

static inline void lexer_print_caret_line(i32 column, i32 width) {
    i32 i = 0;
    fprintf(stderr, "    ");
    for (i = 1; i < column; i++) {
        fputc(' ', stderr);
    }
    for (i = 0; i < width; i++) {
        fputc('^', stderr);
    }
    fputc('\n', stderr);
}

static inline const char *lexer_token_name(i32 token) {
    switch (token) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_BREAK: return "BREAK";
        case TOKEN_CASE: return "CASE";
        case TOKEN_DEFAULT: return "DEFAULT";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_IF: return "IF";
        case TOKEN_INPUT: return "INPUT";
        case TOKEN_INT: return "INT";
        case TOKEN_OUTPUT: return "OUTPUT";
        case TOKEN_SWITCH: return "SWITCH";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_COMMA: return "COMMA";
        case TOKEN_COLON: return "COLON";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_RELOP_EQ: return "RELOP_EQ";
        case TOKEN_RELOP_NE: return "RELOP_NE";
        case TOKEN_RELOP_LT: return "RELOP_LT";
        case TOKEN_RELOP_GT: return "RELOP_GT";
        case TOKEN_RELOP_GE: return "RELOP_GE";
        case TOKEN_RELOP_LE: return "RELOP_LE";
        case TOKEN_ADDOP_PLUS: return "ADDOP_PLUS";
        case TOKEN_ADDOP_MINUS: return "ADDOP_MINUS";
        case TOKEN_MULOP_MUL: return "MULOP_MUL";
        case TOKEN_MULOP_DIV: return "MULOP_DIV";
        case TOKEN_OR: return "OR";
        case TOKEN_AND: return "AND";
        case TOKEN_NOT: return "NOT";
        case TOKEN_CAST_INT: return "CAST_INT";
        case TOKEN_CAST_FLOAT: return "CAST_FLOAT";
        case TOKEN_ID: return "ID";
        case TOKEN_NUM_INT: return "NUM_INT";
        case TOKEN_NUM_FLOAT: return "NUM_FLOAT";
        default: return "UNKNOWN";
    }
}

#endif

