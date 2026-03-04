#include "ast.h"

#include <stdlib.h>

static void *ast_alloc(i32 size) {
    return calloc(1, (size_t)size);
}

// for idlist -> ID, create a new identifier in the AST 
AstIdentifier *new_ast_identifier(char *name) {
    AstIdentifier *node = (AstIdentifier *) ast_alloc(sizeof(AstIdentifier));
    if (node == NULL) {
        free(name);
        return NULL;
    }

    node->name = name;
    return node;
}

// for idlist -> idlist ',' ID, append an identifier to the end of the identifier list
AstIdentifier *ast_identifier_append(AstIdentifier *head, AstIdentifier *node) {
    AstIdentifier *it = head;
    if (head == NULL) {
        return node;
    }

    // TODO: This traversal to the end is not efficient enough. Maybe track the end?
    while (it->next != NULL) {
        it = it->next;
    }

    it->next = node;
    return head;
}

// free all the identifiers in the list up to now
void free_ast_identifier(AstIdentifier *head) {
    AstIdentifier *next = NULL;
    while (head != NULL) {
        next = head->next;
        free(head->name);
        free(head);
        head = next;
    }
}

// for declaration -> idlist ':' type ';', create a new declaration node with a given
// identifiers list and the type
AstDeclaration *new_ast_declaration(AstIdentifier *identifiers, AstType type) {
    AstDeclaration *node = (AstDeclaration *) ast_alloc(sizeof(AstDeclaration));
    if (node == NULL) {
        free_ast_identifier(identifiers);
        return NULL;
    }
    node->identifiers = identifiers;
    node->type = type;
    return node;
}

// for declarations -> declarations declaration. Append a new declaration node to the end
AstDeclaration *ast_declaration_append(AstDeclaration *head, AstDeclaration *node) {
    AstDeclaration *it = head;
    if (head == NULL) {
        return node;
    }

    // TODO: This traversal to the end is not efficient enough. Maybe track the end?
    while (it->next != NULL) {
        it = it->next;
    }
    it->next = node;
    return head;
}

// free the declaration lists (and their inner identifiers) */
void free_ast_declaration(AstDeclaration *head) {
    AstDeclaration *next = NULL;
    while (head != NULL) {
        next = head->next;
        free_ast_identifier(head->identifiers);
        free(head);
        head = next;
    }
}

// for factor -> ID, create a new AST_EXPR_ID expression node with an identifier
AstExpression *new_ast_expr_id(char *identifier) {
    AstExpression *node = (AstExpression *) ast_alloc(sizeof(AstExpression));
    if (node == NULL) {
        free(identifier);
        return NULL;
    }
    node->kind = AST_EXPR_ID;
    node->as.identifier = identifier;
    return node;
}

// for factor -> NUM_INT, create a AST_EXPR_INT_LITERAL expression with a given number
// we store it as text for now (TODO: Explain why)
AstExpression *new_ast_expr_int_literal(char *number_text) {
    AstExpression *node = (AstExpression *) ast_alloc(sizeof(AstExpression));
    if (node == NULL) {
        free(number_text);
        return NULL;
    }
    node->kind = AST_EXPR_INT_LITERAL;
    node->as.number_text = number_text;
    return node;
}

// for factor -> NUM_FLOAT, create a AST_EXPR_FLOAT_LITERAL expression with a given number
AstExpression *new_ast_expr_float_literal(char *number_text) {
    AstExpression *node = (AstExpression *) ast_alloc(sizeof(AstExpression));
    if (node == NULL) {
        free(number_text);
        return NULL;
    }
    node->kind = AST_EXPR_FLOAT_LITERAL;
    node->as.number_text = number_text;
    return node;
}

// for factor -> CAST_INT '(' expression ')', create a AST_EXPR_CAST_INT expression with a given operand
// the operand itself could be any ast defined expression
AstExpression *new_ast_expr_cast_int(AstExpression *operand) {
    AstExpression *node = (AstExpression *) ast_alloc(sizeof(AstExpression));
    if (node == NULL) {
        free_ast_expression(operand);
        return NULL;
    }
    node->kind = AST_EXPR_CAST_INT;
    node->as.unary.operand = operand;
    return node;
}

// for factor -> CAST_FLOAT '(' expression ')', create a AST_EXPR_CAST_FLOAT expression with a given operand
AstExpression *new_ast_expr_cast_float(AstExpression *operand) {
    AstExpression *node = (AstExpression *)ast_alloc(sizeof(AstExpression));
    if (node == NULL) {
        free_ast_expression(operand);
        return NULL;
    }
    node->kind = AST_EXPR_CAST_FLOAT;
    node->as.unary.operand = operand;
    return node;
}

// for boolfactor -> NOT '(' boolexpr ')', create a AST_EXPR_NOT expression with a given operand
AstExpression *new_ast_expr_not(AstExpression *operand) {
    AstExpression *node = (AstExpression *)ast_alloc(sizeof(AstExpression));
    if (node == NULL) {
        free_ast_expression(operand);
        return NULL;
    }
    node->kind = AST_EXPR_NOT;
    node->as.unary.operand = operand;
    return node;
}

/*
 * For any binary operands:
 * 
 *   boolexpr   -> boolexpr OR boolterm
 *   boolterm   -> boolterm AND boolfactor
 *   boolfactor -> expression RELOP expression
 *   expression -> expression ADDOP term
 *   term       -> term MULOP factor
 * add a expression node that represents the operation, left and right operands
*/
AstExpression *new_ast_expr_binary(AstBinaryOp op, AstExpression *left, AstExpression *right) {
    AstExpression *node = (AstExpression *)ast_alloc(sizeof(AstExpression));
    if (node == NULL) {
        free_ast_expression(left);
        free_ast_expression(right);
        return NULL;
    }
    node->kind = AST_EXPR_BINARY;
    node->as.binary.op = op;
    node->as.binary.left = left;
    node->as.binary.right = right;
    return node;
}

// Free an expression (any kind)
void free_ast_expression(AstExpression *expression) {
    if (expression == NULL) {
        return;
    }

    switch (expression->kind) {
        case AST_EXPR_ID:
            free(expression->as.identifier);
            break;
        case AST_EXPR_INT_LITERAL:
        case AST_EXPR_FLOAT_LITERAL:
            free(expression->as.number_text);
            break;
        case AST_EXPR_CAST_INT:
        case AST_EXPR_CAST_FLOAT:
        case AST_EXPR_NOT:
            free_ast_expression(expression->as.unary.operand);
            break;
        case AST_EXPR_BINARY:
            free_ast_expression(expression->as.binary.left);
            free_ast_expression(expression->as.binary.right);
            break;
        default:
            break;
    }

    free(expression);
}

// for case_item -> CASE NUM_INT ':' stmtlist, create a new case node
AstCase *new_ast_case(char *int_literal, AstStatement *statements) {
    AstCase *node = (AstCase *)ast_alloc(sizeof(AstCase));
    if (node == NULL) {
        free(int_literal);
        free_ast_statement(statements);
        return NULL;
    }
    node->int_literal = int_literal;
    node->statements = statements;
    return node;
}

// for caselist -> caselist case_item, append a new case node to the list
AstCase *ast_case_append(AstCase *head, AstCase *node) {
    AstCase *it = head;
    if (head == NULL) {
        return node;
    }

    // TODO: This traversal to the end is not efficient enough. Maybe track the end?
    while (it->next != NULL) {
        it = it->next;
    }
    it->next = node;
    return head;
}

// free case lists and their nested statement lists
void free_ast_case(AstCase *head) {
    AstCase *next = NULL;
    while (head != NULL) {
        next = head->next;
        free(head->int_literal);
        free_ast_statement(head->statements);
        free(head);
        head = next;
    }
}

// for assignment_stmt -> ID '=' expression ';', create a new assign statement node
AstStatement *new_ast_statement_assign(char *identifier, AstExpression *expression) {
    AstStatement *node = (AstStatement *)ast_alloc(sizeof(AstStatement));
    if (node == NULL) {
        free(identifier);
        free_ast_expression(expression);
        return NULL;
    }
    node->kind = AST_STMT_ASSIGN;
    node->as.assign_stmt.identifier = identifier;
    node->as.assign_stmt.expression = expression;
    return node;
}

// for input_stmt -> INPUT '(' ID ')' ';', create a new statement node for input
AstStatement *new_ast_statement_input(char *identifier) {
    AstStatement *node = (AstStatement *)ast_alloc(sizeof(AstStatement));
    if (node == NULL) {
        free(identifier);
        return NULL;
    }
    node->kind = AST_STMT_INPUT;
    node->as.input_stmt.identifier = identifier;
    return node;
}

// for output_stmt -> OUTPUT '(' expression ')' ';', new statement node for output
AstStatement *new_ast_statement_output(AstExpression *expression) {
    AstStatement *node = (AstStatement *)ast_alloc(sizeof(AstStatement));
    if (node == NULL) {
        free_ast_expression(expression);
        return NULL;
    }
    node->kind = AST_STMT_OUTPUT;
    node->as.output_stmt.expression = expression;
    return node;
}

/*
 * for:
 *   if_stmt -> IF '(' boolexpr ')' stmt
 *   if_stmt -> IF '(' boolexpr ')' stmt ELSE stmt
 */
AstStatement *new_ast_statement_if(AstExpression *condition, AstStatement *then_stmt, AstStatement *else_stmt) {
    // if this conditional doesn't have a else part, it will be NULL
    AstStatement *node = (AstStatement *)ast_alloc(sizeof(AstStatement));
    if (node == NULL) {
        free_ast_expression(condition);
        free_ast_statement(then_stmt);
        free_ast_statement(else_stmt);
        return NULL;
    }
    node->kind = AST_STMT_IF;
    node->as.if_stmt.condition = condition;
    node->as.if_stmt.then_stmt = then_stmt;
    node->as.if_stmt.else_stmt = else_stmt;
    return node;
}

// for while_stmt -> WHILE '(' boolexpr ')' stmt. Create a new while statement node 
AstStatement *new_ast_statement_while(AstExpression *condition, AstStatement *body) {
    AstStatement *node = (AstStatement *)ast_alloc(sizeof(AstStatement));
    if (node == NULL) {
        free_ast_expression(condition);
        free_ast_statement(body);
        return NULL;
    }
    node->kind = AST_STMT_WHILE;
    node->as.while_stmt.condition = condition;
    node->as.while_stmt.body = body;
    return node;
}

// for switch_stmt -> SWITCH '(' expression ')' '{' caselist DEFAULT ':' stmtlist '}'
AstStatement *new_ast_statement_switch(AstExpression *expression, AstCase *cases, AstStatement *default_statements) {
    AstStatement *node = (AstStatement *)ast_alloc(sizeof(AstStatement));
    if (node == NULL) {
        free_ast_expression(expression);
        free_ast_case(cases);
        free_ast_statement(default_statements);
        return NULL;
    }
    node->kind = AST_STMT_SWITCH;
    node->as.switch_stmt.expression = expression;
    node->as.switch_stmt.cases = cases;
    node->as.switch_stmt.default_statements = default_statements;
    return node;
}

/* for break_stmt -> BREAK ';' */
AstStatement *new_ast_statement_break(void) {
    AstStatement *node = (AstStatement *)ast_alloc(sizeof(AstStatement));
    if (node == NULL) {
        return NULL;
    }
    node->kind = AST_STMT_BREAK;
    return node;
}

// for stmt_block -> '{' stmtlist '}'
AstStatement *new_ast_statement_block(AstStatement *statements) {
    AstStatement *node = (AstStatement *)ast_alloc(sizeof(AstStatement));
    if (node == NULL) {
        free_ast_statement(statements);
        return NULL;
    }
    node->kind = AST_STMT_BLOCK;
    node->as.block_stmt.statements = statements;
    return node;
}

// for stmtlist -> stmtlist stmt
AstStatement *ast_statement_append(AstStatement *head, AstStatement *node) {
    AstStatement *it = head;
    if (head == NULL) {
        return node;
    }

    // TODO: This traversal to the end is not efficient enough. Maybe track the end?
    while (it->next != NULL) {
        it = it->next;
    }
    it->next = node;
    return head;
}

// free statement lists child nodes recursive
void free_ast_statement(AstStatement *head) {
    AstStatement *next = NULL;
    while (head != NULL) {
        next = head->next;
        switch (head->kind) {
            case AST_STMT_ASSIGN:
                free(head->as.assign_stmt.identifier);
                free_ast_expression(head->as.assign_stmt.expression);
                break;
            case AST_STMT_INPUT:
                free(head->as.input_stmt.identifier);
                break;
            case AST_STMT_OUTPUT:
                free_ast_expression(head->as.output_stmt.expression);
                break;
            case AST_STMT_IF:
                free_ast_expression(head->as.if_stmt.condition);
                free_ast_statement(head->as.if_stmt.then_stmt);
                free_ast_statement(head->as.if_stmt.else_stmt);
                break;
            case AST_STMT_WHILE:
                free_ast_expression(head->as.while_stmt.condition);
                free_ast_statement(head->as.while_stmt.body);
                break;
            case AST_STMT_SWITCH:
                free_ast_expression(head->as.switch_stmt.expression);
                free_ast_case(head->as.switch_stmt.cases);
                free_ast_statement(head->as.switch_stmt.default_statements);
                break;
            case AST_STMT_BREAK:
                break;
            case AST_STMT_BLOCK:
                free_ast_statement(head->as.block_stmt.statements);
                break;
            default:
                break;
        }
        free(head);
        head = next;
    }
}

// for program -> declarations stmt_block
AstProgram *new_ast_program(AstDeclaration *declarations, AstStatement *body) {
    AstProgram *program = (AstProgram *)ast_alloc(sizeof(AstProgram));
    if (program == NULL) {
        free_ast_declaration(declarations);
        free_ast_statement(body);
        return NULL;
    }
    program->declarations = declarations;
    program->body = body;
    return program;
}

// free the full ast
void free_ast_program(AstProgram *program) {
    if (program == NULL) {
        return;
    }

    free_ast_declaration(program->declarations);
    free_ast_statement(program->body);
    free(program);
}
