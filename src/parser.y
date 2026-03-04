%code requires {
#include "ast.h"
#include "defs.h"
}

%{
#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lexer.h"
#include "log.h"

extern FILE *yyin;
int yylex(void);
void yyerror(const char *message);

static AstProgram *parsed_program = NULL;
static i32 parse_error_count = 0;
%}

%define parse.error verbose
%expect 1

%union {
    char *text;
    AstType type;
    AstBinaryOp binary_op;
    AstDeclaration *declaration;
    AstIdentifier *identifier;
    AstStatement *statement;
    AstExpression *expression;
    AstCase *case_node;
}

%token TOKEN_BREAK TOKEN_CASE TOKEN_DEFAULT TOKEN_ELSE TOKEN_FLOAT TOKEN_IF TOKEN_INPUT TOKEN_INT TOKEN_OUTPUT TOKEN_SWITCH TOKEN_WHILE
%token TOKEN_LPAREN TOKEN_RPAREN TOKEN_LBRACE TOKEN_RBRACE TOKEN_COMMA TOKEN_COLON TOKEN_SEMICOLON TOKEN_ASSIGN
%token TOKEN_RELOP_EQ TOKEN_RELOP_NE TOKEN_RELOP_LT TOKEN_RELOP_GT TOKEN_RELOP_GE TOKEN_RELOP_LE
%token TOKEN_ADDOP_PLUS TOKEN_ADDOP_MINUS TOKEN_MULOP_MUL TOKEN_MULOP_DIV TOKEN_OR TOKEN_AND TOKEN_NOT
%token TOKEN_CAST_INT TOKEN_CAST_FLOAT
%token <text> TOKEN_ID TOKEN_NUM_INT TOKEN_NUM_FLOAT

%type <declaration> declarations declaration
%type <identifier> idlist
%type <type> type
%type <statement> stmt assignment_stmt input_stmt output_stmt if_stmt while_stmt switch_stmt break_stmt stmt_block stmtlist
%type <expression> boolexpr boolterm boolfactor expression term factor
%type <binary_op> relop
%type <case_node> caselist case_item

%destructor { free($$); } <text>
%destructor { free_ast_declaration($$); } <declaration>
%destructor { free_ast_identifier($$); } <identifier>
%destructor { free_ast_statement($$); } <statement>
%destructor { free_ast_expression($$); } <expression>
%destructor { free_ast_case($$); } <case_node>

%nonassoc LOWER_THAN_ELSE
%nonassoc TOKEN_ELSE

%start program

%%

program:
      declarations stmt_block
        {
            parsed_program = new_ast_program($1, $2);
            if (parsed_program == NULL) {
                ERROR("out of memory while building AST");
                YYABORT;
            }
        }
    ;

declarations:
      declarations declaration { $$ = ast_declaration_append($1, $2); }
    | /* empty */              { $$ = NULL; }
    ;

declaration:
      idlist TOKEN_COLON type TOKEN_SEMICOLON
        {
            $$ = new_ast_declaration($1, $3);
            if ($$ == NULL) {
                ERROR("out of memory while building declaration node");
                YYABORT;
            }
        }
    | error TOKEN_SEMICOLON
        {
            yyerrok;
            $$ = NULL;
        }
    ;

type:
      TOKEN_INT   { $$ = AST_TYPE_INT; }
    | TOKEN_FLOAT { $$ = AST_TYPE_FLOAT; }
    ;

idlist:
      idlist TOKEN_COMMA TOKEN_ID
        {
            AstIdentifier *item = new_ast_identifier($3);
            if (item == NULL) {
                ERROR("out of memory while building identifier list");
                YYABORT;
            }
            $$ = ast_identifier_append($1, item);
        }
    | TOKEN_ID
        {
            $$ = new_ast_identifier($1);
            if ($$ == NULL) {
                ERROR("out of memory while building identifier");
                YYABORT;
            }
        }
    ;

stmt:
      assignment_stmt { $$ = $1; }
    | input_stmt      { $$ = $1; }
    | output_stmt     { $$ = $1; }
    | if_stmt         { $$ = $1; }
    | while_stmt      { $$ = $1; }
    | switch_stmt     { $$ = $1; }
    | break_stmt      { $$ = $1; }
    | stmt_block      { $$ = $1; }
    | error TOKEN_SEMICOLON
        {
            yyerrok;
            $$ = NULL;
        }
    ;

assignment_stmt:
      TOKEN_ID TOKEN_ASSIGN expression TOKEN_SEMICOLON
        {
            $$ = new_ast_statement_assign($1, $3);
            if ($$ == NULL) {
                ERROR("out of memory while building assignment statement");
                YYABORT;
            }
        }
    | TOKEN_ID TOKEN_ASSIGN error TOKEN_SEMICOLON
        {
            free($1);
            yyerrok;
            $$ = NULL;
        }
    ;

input_stmt:
      TOKEN_INPUT TOKEN_LPAREN TOKEN_ID TOKEN_RPAREN TOKEN_SEMICOLON
        {
            $$ = new_ast_statement_input($3);
            if ($$ == NULL) {
                ERROR("out of memory while building input statement");
                YYABORT;
            }
        }
    | TOKEN_INPUT TOKEN_LPAREN error TOKEN_RPAREN TOKEN_SEMICOLON
        {
            yyerrok;
            $$ = NULL;
        }
    ;

output_stmt:
      TOKEN_OUTPUT TOKEN_LPAREN expression TOKEN_RPAREN TOKEN_SEMICOLON
        {
            $$ = new_ast_statement_output($3);
            if ($$ == NULL) {
                ERROR("out of memory while building output statement");
                YYABORT;
            }
        }
    | TOKEN_OUTPUT TOKEN_LPAREN error TOKEN_RPAREN TOKEN_SEMICOLON
        {
            yyerrok;
            $$ = NULL;
        }
    ;

if_stmt:
      TOKEN_IF TOKEN_LPAREN boolexpr TOKEN_RPAREN stmt %prec LOWER_THAN_ELSE
        {
            $$ = new_ast_statement_if($3, $5, NULL);
            if ($$ == NULL) {
                ERROR("out of memory while building if statement");
                YYABORT;
            }
        }
    | TOKEN_IF TOKEN_LPAREN boolexpr TOKEN_RPAREN stmt TOKEN_ELSE stmt
        {
            $$ = new_ast_statement_if($3, $5, $7);
            if ($$ == NULL) {
                ERROR("out of memory while building if-else statement");
                YYABORT;
            }
        }
    ;

while_stmt:
      TOKEN_WHILE TOKEN_LPAREN boolexpr TOKEN_RPAREN stmt
        {
            $$ = new_ast_statement_while($3, $5);
            if ($$ == NULL) {
                ERROR("out of memory while building while statement");
                YYABORT;
            }
        }
    ;

switch_stmt:
      TOKEN_SWITCH TOKEN_LPAREN expression TOKEN_RPAREN TOKEN_LBRACE caselist TOKEN_DEFAULT TOKEN_COLON stmtlist TOKEN_RBRACE
        {
            $$ = new_ast_statement_switch($3, $6, $9);
            if ($$ == NULL) {
                ERROR("out of memory while building switch statement");
                YYABORT;
            }
        }
    | TOKEN_SWITCH TOKEN_LPAREN expression TOKEN_RPAREN TOKEN_LBRACE error TOKEN_RBRACE
        {
            free_ast_expression($3);
            yyerrok;
            $$ = NULL;
        }
    ;

caselist:
      caselist case_item { $$ = ast_case_append($1, $2); }
    | /* empty */        { $$ = NULL; }
    ;

case_item:
      TOKEN_CASE TOKEN_NUM_INT TOKEN_COLON stmtlist
        {
            $$ = new_ast_case($2, $4);
            if ($$ == NULL) {
                ERROR("out of memory while building case node");
                YYABORT;
            }
        }
    ;

break_stmt:
      TOKEN_BREAK TOKEN_SEMICOLON
        {
            $$ = new_ast_statement_break();
            if ($$ == NULL) {
                ERROR("out of memory while building break statement");
                YYABORT;
            }
        }
    | TOKEN_BREAK error TOKEN_SEMICOLON
        {
            yyerrok;
            $$ = NULL;
        }
    ;

stmt_block:
      TOKEN_LBRACE stmtlist TOKEN_RBRACE
        {
            $$ = new_ast_statement_block($2);
            if ($$ == NULL) {
                ERROR("out of memory while building block statement");
                YYABORT;
            }
        }
    | TOKEN_LBRACE error TOKEN_RBRACE
        {
            yyerrok;
            $$ = new_ast_statement_block(NULL);
            if ($$ == NULL) {
                ERROR("out of memory while building recovered block statement");
                YYABORT;
            }
        }
    ;

stmtlist:
      stmtlist stmt { $$ = ast_statement_append($1, $2); }
    | /* empty */   { $$ = NULL; }
    ;

boolexpr:
      boolexpr TOKEN_OR boolterm
        {
            $$ = new_ast_expr_binary(AST_BINOP_OR, $1, $3);
            if ($$ == NULL) {
                ERROR("out of memory while building OR expression");
                YYABORT;
            }
        }
    | boolterm { $$ = $1; }
    ;

boolterm:
      boolterm TOKEN_AND boolfactor
        {
            $$ = new_ast_expr_binary(AST_BINOP_AND, $1, $3);
            if ($$ == NULL) {
                ERROR("out of memory while building AND expression");
                YYABORT;
            }
        }
    | boolfactor { $$ = $1; }
    ;

boolfactor:
      TOKEN_NOT TOKEN_LPAREN boolexpr TOKEN_RPAREN
        {
            $$ = new_ast_expr_not($3);
            if ($$ == NULL) {
                ERROR("out of memory while building NOT expression");
                YYABORT;
            }
        }
    | expression relop expression
        {
            $$ = new_ast_expr_binary($2, $1, $3);
            if ($$ == NULL) {
                ERROR("out of memory while building relational expression");
                YYABORT;
            }
        }
    ;

relop:
      TOKEN_RELOP_EQ { $$ = AST_BINOP_EQ; }
    | TOKEN_RELOP_NE { $$ = AST_BINOP_NE; }
    | TOKEN_RELOP_LT { $$ = AST_BINOP_LT; }
    | TOKEN_RELOP_GT { $$ = AST_BINOP_GT; }
    | TOKEN_RELOP_GE { $$ = AST_BINOP_GE; }
    | TOKEN_RELOP_LE { $$ = AST_BINOP_LE; }
    ;

expression:
      expression TOKEN_ADDOP_PLUS term
        {
            $$ = new_ast_expr_binary(AST_BINOP_ADD, $1, $3);
            if ($$ == NULL) {
                ERROR("out of memory while building addition expression");
                YYABORT;
            }
        }
    | expression TOKEN_ADDOP_MINUS term
        {
            $$ = new_ast_expr_binary(AST_BINOP_SUB, $1, $3);
            if ($$ == NULL) {
                ERROR("out of memory while building subtraction expression");
                YYABORT;
            }
        }
    | term { $$ = $1; }
    ;

term:
      term TOKEN_MULOP_MUL factor
        {
            $$ = new_ast_expr_binary(AST_BINOP_MUL, $1, $3);
            if ($$ == NULL) {
                ERROR("out of memory while building multiplication expression");
                YYABORT;
            }
        }
    | term TOKEN_MULOP_DIV factor
        {
            $$ = new_ast_expr_binary(AST_BINOP_DIV, $1, $3);
            if ($$ == NULL) {
                ERROR("out of memory while building division expression");
                YYABORT;
            }
        }
    | factor { $$ = $1; }
    ;

factor:
      TOKEN_LPAREN expression TOKEN_RPAREN { $$ = $2; }
    | TOKEN_CAST_INT TOKEN_LPAREN expression TOKEN_RPAREN
        {
            $$ = new_ast_expr_cast_int($3);
            if ($$ == NULL) {
                ERROR("out of memory while building cast<int> expression");
                YYABORT;
            }
        }
    | TOKEN_CAST_FLOAT TOKEN_LPAREN expression TOKEN_RPAREN
        {
            $$ = new_ast_expr_cast_float($3);
            if ($$ == NULL) {
                ERROR("out of memory while building cast<float> expression");
                YYABORT;
            }
        }
    | TOKEN_ID
        {
            $$ = new_ast_expr_id($1);
            if ($$ == NULL) {
                ERROR("out of memory while building identifier expression");
                YYABORT;
            }
        }
    | TOKEN_NUM_INT
        {
            $$ = new_ast_expr_int_literal($1);
            if ($$ == NULL) {
                ERROR("out of memory while building integer literal expression");
                YYABORT;
            }
        }
    | TOKEN_NUM_FLOAT
        {
            $$ = new_ast_expr_float_literal($1);
            if ($$ == NULL) {
                ERROR("out of memory while building float literal expression");
                YYABORT;
            }
        }
    ;

%%

void yyerror(const char *message) {
    const char *near_text = lexer_text();
    const char *line_text = lexer_line_preview();
    i32 width = 1;

    if (near_text != NULL && near_text[0] != '\0') {
        width = (i32)strlen(near_text);
    }
    width = MAX(width, 1);

    if (near_text != NULL && near_text[0] != '\0') {
        ERROR("line %d:%d: %s near '%s'", (int)lexer_line(), (int)lexer_column(), message, near_text);
    } else {
        ERROR("line %d:%d: %s", (int)lexer_line(), (int)lexer_column(), message);
    }
    if (line_text != NULL) {
        fprintf(stderr, "    %s\n", line_text);
        lexer_print_caret_line(lexer_column(), width);
    }
    parse_error_count++;
}

i32 parser_parse(FILE *input_file, AstProgram **out_program, i32 *out_error_count) {
    i32 yy_status = 0;
    i32 total_errors = 0;

    if (out_program != NULL) {
        *out_program = NULL;
    }
    if (out_error_count != NULL) {
        *out_error_count = 0;
    }

    if (!lexer_begin(input_file)) {
        ERROR("failed to initialize lexer for parsing");
        if (out_error_count != NULL) {
            *out_error_count = 1;
        }
        return 1;
    }

    parsed_program = NULL;
    parse_error_count = 0;
    yy_status = yyparse();

    total_errors = parse_error_count + lexer_error_count();
    if (yy_status != 0 && parse_error_count == 0) {
        total_errors++;
    }

    lexer_end();

    if (total_errors > 0) {
        free_ast_program(parsed_program);
        parsed_program = NULL;
        if (out_error_count != NULL) {
            *out_error_count = total_errors;
        }
        return 1;
    }

    if (out_program != NULL) {
        *out_program = parsed_program;
    } else {
        free_ast_program(parsed_program);
    }

    if (out_error_count != NULL) {
        *out_error_count = 0;
    }
    return 0;
}
