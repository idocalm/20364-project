#ifndef AST_H
#define AST_H

#include "defs.h"

/* CPL scalar type tags for declarations and later semantic typing. */
typedef enum ast_type {
    AST_TYPE_INT = 1,
    AST_TYPE_FLOAT = 2
} AstType;

// All the operands defined in CPL: +, -, *, /, ||, etc 
typedef enum ast_binary_op {
    AST_BINOP_ADD,
    AST_BINOP_SUB,
    AST_BINOP_MUL,
    AST_BINOP_DIV,
    AST_BINOP_OR,
    AST_BINOP_AND,
    AST_BINOP_EQ,
    AST_BINOP_NE,
    AST_BINOP_LT,
    AST_BINOP_GT,
    AST_BINOP_GE,
    AST_BINOP_LE
} AstBinaryOp;

/* TODO */
typedef enum ast_expr_kind {
    AST_EXPR_ID,
    AST_EXPR_INT_LITERAL,
    AST_EXPR_FLOAT_LITERAL,
    AST_EXPR_CAST_INT,
    AST_EXPR_CAST_FLOAT,
    AST_EXPR_NOT,
    AST_EXPR_BINARY
} AstExprKind;

// A statement can be either assignment, input, output, if, while, switch, break or stmt block
typedef enum ast_stmt_kind {
    AST_STMT_ASSIGN,
    AST_STMT_INPUT,
    AST_STMT_OUTPUT,
    AST_STMT_IF,
    AST_STMT_WHILE,
    AST_STMT_SWITCH,
    AST_STMT_BREAK,
    AST_STMT_BLOCK
} AstStmtKind;

typedef struct ast_expression AstExpression;
typedef struct ast_statement AstStatement;
typedef struct ast_identifier AstIdentifier;
typedef struct ast_declaration AstDeclaration;
typedef struct ast_case AstCase;
typedef struct ast_program AstProgram;

/*
 * Linked list for identifiers in declarations
 *   idlist -> ID
 *   idlist -> idlist ',' ID
 */
struct ast_identifier {
    char *name;
    AstIdentifier *next;
};

/*
 * Linked list of declarations
 *   declaration  -> idlist ':' type ';'
 *   declarations -> declarations declaration | epsilon
 */
struct ast_declaration {
    AstIdentifier *identifiers;
    AstType type; // declaration type: int or float
    AstDeclaration *next;
};

/*
 * Expression subtree
 *   factor, term, expression, boolfactor, boolterm, boolexpr
 */
struct ast_expression {
    AstExprKind kind;
    union {
        char *identifier;
        char *number_text;
        struct {
            AstExpression *operand;
        } unary;
        struct {
            AstBinaryOp op;
            AstExpression *left;
            AstExpression *right;
        } binary;
    } as;
};

/*
 * Linked list for switch case items
 *   case_item -> CASE NUM_INT ':' stmtlist
 *   caselist  -> caselist case_item | epsilon
 */
struct ast_case {
    char *int_literal; // NUM_INT
    AstStatement *statements;
    AstCase *next;
};

/*
 * TODO
 * Statement node with sibling chaining via `next`.
 * Grammar origin:
 *   stmt, stmtlist, stmt_block, assignment_stmt, input_stmt, output_stmt,
 *   if_stmt, while_stmt, switch_stmt, break_stmt
 */
struct ast_statement {
    AstStmtKind kind;
    AstStatement *next;
    union {
        struct {
            char *identifier;
            AstExpression *expression;
        } assign_stmt;
        struct {
            char *identifier;
        } input_stmt;
        struct {
            AstExpression *expression;
        } output_stmt;
        struct {
            AstExpression *condition;
            AstStatement *then_stmt;
            AstStatement *else_stmt;
        } if_stmt;
        struct {
            AstExpression *condition;
            AstStatement *body;
        } while_stmt;
        struct {
            AstExpression *expression;
            AstCase *cases;
            AstStatement *default_statements;
        } switch_stmt;
        struct {
            AstStatement *statements;
        } block_stmt;
    } as;
};

/*
 * The program root
 *   program -> declarations stmt_block
 */
struct ast_program {
    AstDeclaration *declarations; // the declarations of the program
    AstStatement *body; // the body
};

AstIdentifier *new_ast_identifier(char *name);
AstIdentifier *ast_identifier_append(AstIdentifier *head, AstIdentifier *node);
void free_ast_identifier(AstIdentifier *head);

AstDeclaration *new_ast_declaration(AstIdentifier *identifiers, AstType type);
AstDeclaration *ast_declaration_append(AstDeclaration *head, AstDeclaration *node);
void free_ast_declaration(AstDeclaration *head);

AstExpression *new_ast_expr_id(char *identifier);
AstExpression *new_ast_expr_int_literal(char *number_text);
AstExpression *new_ast_expr_float_literal(char *number_text);
AstExpression *new_ast_expr_cast_int(AstExpression *operand);
AstExpression *new_ast_expr_cast_float(AstExpression *operand);
AstExpression *new_ast_expr_not(AstExpression *operand);
AstExpression *new_ast_expr_binary(AstBinaryOp op, AstExpression *left, AstExpression *right);
void free_ast_expression(AstExpression *expression);

AstCase *new_ast_case(char *int_literal, AstStatement *statements);
AstCase *ast_case_append(AstCase *head, AstCase *node);
void free_ast_case(AstCase *head);

AstStatement *new_ast_statement_assign(char *identifier, AstExpression *expression);
AstStatement *new_ast_statement_input(char *identifier);
AstStatement *new_ast_statement_output(AstExpression *expression);
AstStatement *new_ast_statement_if(AstExpression *condition, AstStatement *then_stmt, AstStatement *else_stmt);
AstStatement *new_ast_statement_while(AstExpression *condition, AstStatement *body);
AstStatement *new_ast_statement_switch(AstExpression *expression, AstCase *cases, AstStatement *default_statements);
AstStatement *new_ast_statement_break(void);
AstStatement *new_ast_statement_block(AstStatement *statements);
AstStatement *ast_statement_append(AstStatement *head, AstStatement *node);
void free_ast_statement(AstStatement *head);

AstProgram *new_ast_program(AstDeclaration *declarations, AstStatement *body);
void free_ast_program(AstProgram *program);

#endif
