#include "debug.h"

#include <stdio.h>

static void dbg_indent(FILE *out, int depth) {
    int i = 0;
    for (i = 0; i < depth; i++) {
        fputs("  ", out);
    }
}

static const char *dbg_type_name(AstType type) {
    switch (type) {
        case AST_TYPE_INT: return "int";
        case AST_TYPE_FLOAT: return "float";
        default: return "unknown_type";
    }
}

static const char *dbg_binop_name(AstBinaryOp op) {
    switch (op) {
        case AST_BINOP_ADD: return "+";
        case AST_BINOP_SUB: return "-";
        case AST_BINOP_MUL: return "*";
        case AST_BINOP_DIV: return "/";
        case AST_BINOP_OR: return "||";
        case AST_BINOP_AND: return "&&";
        case AST_BINOP_EQ: return "==";
        case AST_BINOP_NE: return "!=";
        case AST_BINOP_LT: return "<";
        case AST_BINOP_GT: return ">";
        case AST_BINOP_GE: return ">=";
        case AST_BINOP_LE: return "<=";
        default: return "?";
    }
}

static void dbg_print_expression(FILE *out, const AstExpression *expr, int depth);
static void dbg_print_statement_list(FILE *out, const AstStatement *stmt, int depth);

static void dbg_print_identifier_list(FILE *out, const AstIdentifier *id, int depth) {
    dbg_indent(out, depth);
    fputs("identifiers:\n", out);

    while (id != NULL) {
        dbg_indent(out, depth + 1);
        fprintf(out, "- %s\n", id->name != NULL ? id->name : "<null-id>");
        id = id->next;
    }
}

static void dbg_print_declaration_list(FILE *out, const AstDeclaration *decl, int depth) {
    if (decl == NULL) {
        dbg_indent(out, depth);
        fputs("declarations: <empty>\n", out);
        return;
    }

    dbg_indent(out, depth);
    fputs("declarations:\n", out);
    while (decl != NULL) {
        dbg_indent(out, depth + 1);
        fprintf(out, "declaration (type=%s)\n", dbg_type_name(decl->type));
        dbg_print_identifier_list(out, decl->identifiers, depth + 2);
        decl = decl->next;
    }
}

static void dbg_print_case_list(FILE *out, const AstCase *case_node, int depth) {
    if (case_node == NULL) {
        dbg_indent(out, depth);
        fputs("cases: <empty>\n", out);
        return;
    }

    dbg_indent(out, depth);
    fputs("cases:\n", out);
    while (case_node != NULL) {
        dbg_indent(out, depth + 1);
        fprintf(
            out,
            "case %s\n",
            case_node->int_literal != NULL ? case_node->int_literal : "<null-case-value>"
        );
        dbg_print_statement_list(out, case_node->statements, depth + 2);
        case_node = case_node->next;
    }
}

static void dbg_print_expression(FILE *out, const AstExpression *expr, int depth) {
    if (expr == NULL) {
        dbg_indent(out, depth);
        fputs("<null-expr>\n", out);
        return;
    }

    switch (expr->kind) {
        case AST_EXPR_ID:
            dbg_indent(out, depth);
            fprintf(out, "id(%s)\n", expr->as.identifier != NULL ? expr->as.identifier : "<null-id>");
            break;

        case AST_EXPR_INT_LITERAL:
            dbg_indent(out, depth);
            fprintf(
                out,
                "int_lit(%s)\n",
                expr->as.number_text != NULL ? expr->as.number_text : "<null-int>"
            );
            break;

        case AST_EXPR_FLOAT_LITERAL:
            dbg_indent(out, depth);
            fprintf(
                out,
                "float_lit(%s)\n",
                expr->as.number_text != NULL ? expr->as.number_text : "<null-float>"
            );
            break;

        case AST_EXPR_CAST_INT:
            dbg_indent(out, depth);
            fputs("cast<int>\n", out);
            dbg_print_expression(out, expr->as.unary.operand, depth + 1);
            break;

        case AST_EXPR_CAST_FLOAT:
            dbg_indent(out, depth);
            fputs("cast<float>\n", out);
            dbg_print_expression(out, expr->as.unary.operand, depth + 1);
            break;

        case AST_EXPR_NOT:
            dbg_indent(out, depth);
            fputs("not\n", out);
            dbg_print_expression(out, expr->as.unary.operand, depth + 1);
            break;

        case AST_EXPR_BINARY:
            dbg_indent(out, depth);
            fprintf(out, "binary(%s)\n", dbg_binop_name(expr->as.binary.op));
            dbg_indent(out, depth + 1);
            fputs("left:\n", out);
            dbg_print_expression(out, expr->as.binary.left, depth + 2);
            dbg_indent(out, depth + 1);
            fputs("right:\n", out);
            dbg_print_expression(out, expr->as.binary.right, depth + 2);
            break;

        default:
            dbg_indent(out, depth);
            fputs("unknown_expr\n", out);
            break;
    }
}

static void dbg_print_statement(FILE *out, const AstStatement *stmt, int depth) {
    if (stmt == NULL) {
        dbg_indent(out, depth);
        fputs("<null-stmt>\n", out);
        return;
    }

    switch (stmt->kind) {
        case AST_STMT_ASSIGN:
            dbg_indent(out, depth);
            fprintf(
                out,
                "assign %s :=\n",
                stmt->as.assign_stmt.identifier != NULL ? stmt->as.assign_stmt.identifier : "<null-id>"
            );
            dbg_print_expression(out, stmt->as.assign_stmt.expression, depth + 1);
            break;

        case AST_STMT_INPUT:
            dbg_indent(out, depth);
            fprintf(
                out,
                "input(%s)\n",
                stmt->as.input_stmt.identifier != NULL ? stmt->as.input_stmt.identifier : "<null-id>"
            );
            break;

        case AST_STMT_OUTPUT:
            dbg_indent(out, depth);
            fputs("output:\n", out);
            dbg_print_expression(out, stmt->as.output_stmt.expression, depth + 1);
            break;

        case AST_STMT_IF:
            dbg_indent(out, depth);
            fputs("if\n", out);
            dbg_indent(out, depth + 1);
            fputs("condition:\n", out);
            dbg_print_expression(out, stmt->as.if_stmt.condition, depth + 2);
            dbg_indent(out, depth + 1);
            fputs("then:\n", out);
            dbg_print_statement(out, stmt->as.if_stmt.then_stmt, depth + 2);
            if (stmt->as.if_stmt.else_stmt != NULL) {
                dbg_indent(out, depth + 1);
                fputs("else:\n", out);
                dbg_print_statement(out, stmt->as.if_stmt.else_stmt, depth + 2);
            }
            break;

        case AST_STMT_WHILE:
            dbg_indent(out, depth);
            fputs("while\n", out);
            dbg_indent(out, depth + 1);
            fputs("condition:\n", out);
            dbg_print_expression(out, stmt->as.while_stmt.condition, depth + 2);
            dbg_indent(out, depth + 1);
            fputs("body:\n", out);
            dbg_print_statement(out, stmt->as.while_stmt.body, depth + 2);
            break;

        case AST_STMT_SWITCH:
            dbg_indent(out, depth);
            fputs("switch\n", out);
            dbg_indent(out, depth + 1);
            fputs("expression:\n", out);
            dbg_print_expression(out, stmt->as.switch_stmt.expression, depth + 2);
            dbg_print_case_list(out, stmt->as.switch_stmt.cases, depth + 1);
            dbg_indent(out, depth + 1);
            fputs("default:\n", out);
            dbg_print_statement_list(out, stmt->as.switch_stmt.default_statements, depth + 2);
            break;

        case AST_STMT_BREAK:
            dbg_indent(out, depth);
            fputs("break\n", out);
            break;

        case AST_STMT_BLOCK:
            dbg_indent(out, depth);
            fputs("block\n", out);
            dbg_print_statement_list(out, stmt->as.block_stmt.statements, depth + 1);
            break;

        default:
            dbg_indent(out, depth);
            fputs("unknown_stmt\n", out);
            break;
    }
}

static void dbg_print_statement_list(FILE *out, const AstStatement *stmt, int depth) {
    if (stmt == NULL) {
        dbg_indent(out, depth);
        fputs("<empty>\n", out);
        return;
    }

    while (stmt != NULL) {
        dbg_print_statement(out, stmt, depth);
        stmt = stmt->next;
    }
}

void debug_print_ast(FILE *out, const AstProgram *program) {
    if (out == NULL) {
        out = stdout;
    }

    if (program == NULL) {
        fputs("AST <null>\n", out);
        return;
    }

    fputs("AST\n", out);
    dbg_print_declaration_list(out, program->declarations, 1);
    dbg_indent(out, 1);
    fputs("body:\n", out);
    dbg_print_statement(out, program->body, 2);
}
