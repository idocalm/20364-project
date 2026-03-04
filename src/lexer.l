%{
#include "parser.tab.h"
#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

extern FILE *yyin;
extern int yylineno;
extern char *yytext;
int yylex(void);
int yylex_destroy(void);

static i32 error_count = 0;
static i32 current_col = 1;
static i32 token_col = 1;
static i32 comment_start_line = 1;
static i32 comment_start_column = 1;

static char line_preview[1024];
static i32 line_preview_len = 0;

static void lexer_print_highlight(i32 line, i32 column, const char *message, const char *lexeme) {
    i32 width = 1;
    if (lexeme != NULL && lexeme[0] != '\0') {
        width = (i32)strlen(lexeme);
    }

    width = MAX(width, 1);

    ERROR("line %d:%d: lexical error: %s", (int)line, (int)column, message);
    fprintf(stderr, "    %s\n", line_preview);
    lexer_print_caret_line(column, width);
}

static void lexer_report_bad_char(i8 c) {
    i8 bad_char_text[2];

    bad_char_text[0] = c;
    bad_char_text[1] = '\0';

    char message[64];
    if (is_char_printable(c)) {
        snprintf(message, sizeof(message), "invalid character '%c'", c);
    } else {
        snprintf(message, sizeof(message), "invalid byte 0x%02X", (unsigned char)c);
    }
    lexer_print_highlight((i32)yylineno, token_col, message, (const char *)bad_char_text);
    error_count++;
}

static char *lexer_dup_text(const char *text) {
    size_t len = strlen(text);
    char *copy = (char *)malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, text, len + 1);
    return copy;
}

#define YY_USER_ACTION                                  \
    do {                                                \
        token_col = current_col;              \
        lexer_capture_preview(                           \
            line_preview,                              \
            &line_preview_len,                         \
            (i32)sizeof(line_preview),                 \
            yytext,                                      \
            (i32)yyleng                                  \
        );                                               \
        current_col += (i32)yyleng;                \
    } while (0);
%}

%option noyywrap
%option nodefault
%option yylineno
%option noinput
%option nounput
%option never-interactive

%x COMMENT

LETTER      [A-Za-z]
DIGIT       [0-9]
ID          {LETTER}({LETTER}|{DIGIT})*
INTNUM      {DIGIT}+
FLOATNUM    {DIGIT}+"."{DIGIT}*

%%

"break"                 { return TOKEN_BREAK; }
"case"                  { return TOKEN_CASE; }
"default"               { return TOKEN_DEFAULT; }
"else"                  { return TOKEN_ELSE; }
"float"                 { return TOKEN_FLOAT; }
"if"                    { return TOKEN_IF; }
"input"                 { return TOKEN_INPUT; }
"int"                   { return TOKEN_INT; }
"output"                { return TOKEN_OUTPUT; }
"switch"                { return TOKEN_SWITCH; }
"while"                 { return TOKEN_WHILE; }

"cast<<int>>"           { return TOKEN_CAST_INT; }
"cast<<float>>"         { return TOKEN_CAST_FLOAT; }

"=="                    { return TOKEN_RELOP_EQ; }
"!="                    { return TOKEN_RELOP_NE; }
">="                    { return TOKEN_RELOP_GE; }
"<="                    { return TOKEN_RELOP_LE; }
"<"                     { return TOKEN_RELOP_LT; }
">"                     { return TOKEN_RELOP_GT; }
"||"                    { return TOKEN_OR; }
"&&"                    { return TOKEN_AND; }
"!"                     { return TOKEN_NOT; }
"+"                     { return TOKEN_ADDOP_PLUS; }
"-"                     { return TOKEN_ADDOP_MINUS; }
"*"                     { return TOKEN_MULOP_MUL; }
"/"                     { return TOKEN_MULOP_DIV; }

"("                     { return TOKEN_LPAREN; }
")"                     { return TOKEN_RPAREN; }
"{"                     { return TOKEN_LBRACE; }
"}"                     { return TOKEN_RBRACE; }
","                     { return TOKEN_COMMA; }
":"                     { return TOKEN_COLON; }
";"                     { return TOKEN_SEMICOLON; }
"="                     { return TOKEN_ASSIGN; }

{FLOATNUM}              {
                            yylval.text = lexer_dup_text(yytext);
                            if (yylval.text == NULL) {
                                ERROR("out of memory while scanning float literal");
                                error_count++;
                                return 0;
                            }
                            return TOKEN_NUM_FLOAT;
                        }
{INTNUM}                {
                            yylval.text = lexer_dup_text(yytext);
                            if (yylval.text == NULL) {
                                ERROR("out of memory while scanning integer literal");
                                error_count++;
                                return 0;
                            }
                            return TOKEN_NUM_INT;
                        }
{ID}                    {
                            yylval.text = lexer_dup_text(yytext);
                            if (yylval.text == NULL) {
                                ERROR("out of memory while scanning identifier");
                                error_count++;
                                return 0;
                            }
                            return TOKEN_ID;
                        }

"/*"                    {
                            comment_start_line = (i32)yylineno;
                            comment_start_column = token_col;
                            BEGIN(COMMENT);
                        }
<COMMENT>"*/"           { BEGIN(INITIAL); }
<COMMENT>\n             { current_col = 1; }
<COMMENT>.              { }
<COMMENT><<EOF>>        {
                            lexer_print_highlight(
                                comment_start_line,
                                comment_start_column,
                                "unterminated comment",
                                "/*"
                            );
                            error_count++;
                            BEGIN(INITIAL);
                            return 0;
                        }

[ \t\r]+                { }
\n                      { current_col = 1; }

.                       {
                            lexer_report_bad_char((i8)yytext[0]);
                        }

<<EOF>>                 { return 0; }

%%

b8 lexer_begin(FILE *input_file) {
    if (input_file == NULL) {
        return false;
    }

    yyin = input_file;
    yylineno = 1;
    current_col = 1;
    token_col = 1;
    comment_start_line = 1;
    comment_start_column = 1;
    error_count = 0;
    line_preview[0] = '\0';
    line_preview_len = 0;
    return true;
}

void lexer_end(void) {
    (void)yylex_destroy();
}

i32 lexer_next(void) {
    return (i32)yylex();
}

i32 lexer_line(void) {
    return yylineno;
}

i32 lexer_column(void) {
    return token_col;
}

const char *lexer_text(void) {
    return yytext;
}

const char *lexer_line_preview(void) {
    return line_preview;
}

i32 lexer_error_count(void) {
    return error_count;
}

