#include "semantics.h"

#include "log.h"
#include "symbols.h"



static void raise_semantic_error(SemanticContext *ctx, const char *fmt, const char *name)
{
    ERROR(fmt, name);
    ctx->error_count++;
}

static void check_expression(SemanticContext *ctx, const AstExpression *expr)
{
    if (expr == NULL) {
        return;
    }

    switch (expr->kind) {
        case AST_EXPR_ID:
            // validate that the identifier is already in the symbol table
            // that means: someone defined it already before using!
            if (symtable_find(ctx->symbols, expr->as.identifier) == NULL)
            {
                raise_semantic_error(ctx, "semantic error: use of undeclared identifier '%s'", expr->as.identifier);
            }
            break;

        case AST_EXPR_INT_LITERAL:
        case AST_EXPR_FLOAT_LITERAL:
            break;

        case AST_EXPR_CAST_INT:
        case AST_EXPR_CAST_FLOAT:
        case AST_EXPR_NOT:
            // validate that the operand follows the semantic rules 
            check_expression(ctx, expr->as.unary.operand);
            break;

        case AST_EXPR_BINARY:
            // vlaidate that both sides follow the semantic rules
            check_expression(ctx, expr->as.binary.left);
            check_expression(ctx, expr->as.binary.right);
            break;

        default:
            break;
    }
}

static void check_statement_list(SemanticContext *ctx, const AstStatement *stmt);

static void check_statement(SemanticContext *ctx, const AstStatement *stmt)
{
    const AstCase *case_it = NULL;
    if (stmt == NULL)
        return;

    switch (stmt->kind) {
        case AST_STMT_ASSIGN:
            // validate that the operand were assigning to is defined in the symbol table
            if (symtable_find(ctx->symbols, stmt->as.assign_stmt.identifier) == NULL)
            {
                raise_semantic_error(
                    ctx,
                    "semantic error: assignment to undeclared identifier '%s'",
                    stmt->as.assign_stmt.identifier);
            }
            // also validate that the expression itself follows the semantic rules
            check_expression(ctx, stmt->as.assign_stmt.expression);
            break;

        case AST_STMT_INPUT:
            // validate that the operand were inputing to is defined
            if (symtable_find(ctx->symbols, stmt->as.input_stmt.identifier) == NULL)
            {
                raise_semantic_error(
                    ctx,
                    "semantic error: input to undeclared identifier '%s'",
                    stmt->as.input_stmt.identifier);
            }
            break;

        case AST_STMT_OUTPUT:
            // validate the output expression follows the semantic rules
            check_expression(ctx, stmt->as.output_stmt.expression);
            break;

        case AST_STMT_IF:
            // validate the if, then else follow the semantic rules 
            check_expression(ctx, stmt->as.if_stmt.condition);
            check_statement(ctx, stmt->as.if_stmt.then_stmt);
            check_statement(ctx, stmt->as.if_stmt.else_stmt);
            break;

        case AST_STMT_WHILE:
            // validate the codition and body follow the semantic rules
            check_expression(ctx, stmt->as.while_stmt.condition);
            check_statement(ctx, stmt->as.while_stmt.body);
            break;

        case AST_STMT_SWITCH:
            // validate the expression follows the rules
            check_expression(ctx, stmt->as.switch_stmt.expression);
            case_it = stmt->as.switch_stmt.cases;
            // check that all cases follow the rules and also the defaults
            while (case_it != NULL) {
                check_statement_list(ctx, case_it->statements);
                case_it = case_it->next;
            }
            check_statement_list(ctx, stmt->as.switch_stmt.default_statements);
            break;

        case AST_STMT_BREAK:
            break;

        case AST_STMT_BLOCK:
            // validate that the statements in the block follow the rules
            check_statement_list(ctx, stmt->as.block_stmt.statements);
            break;

        default:
            break;
    }
}

static void check_statement_list(SemanticContext *ctx, const AstStatement *stmt)
{
    // checking the list = check every element in the list
    while (stmt != NULL) {
        check_statement(ctx, stmt);
        stmt = stmt->next;
    }
}

static void check_declarations(SemanticContext *ctx, const AstDeclaration *decl)
{
    // validate list of declarations
    while (decl != NULL) {
        const AstIdentifier *id = decl->identifiers;
        while (id != NULL) {
            if (symtable_find(ctx->symbols, id->name) != NULL)
            {
                // make sure an identifier is not defined more than once
                raise_semantic_error(ctx, "semantic error: redeclaration of identifier '%s'", id->name);
            }
            else if (!symtable_insert(ctx->symbols, id->name, decl->type))
            { // add to symbol table
                raise_semantic_error(ctx, "semantic error: failed to register identifier '%s'", id->name);
            }
            id = id->next;
        }
        decl = decl->next;
    }
}

i32 semantic_check_program(const AstProgram *program, SymbolTable **out_symbols)
{
    SemanticContext ctx;
    ctx.symbols = NULL;
    ctx.error_count = 0;

    if (out_symbols != NULL) {
        *out_symbols = NULL;
    }

    if (program == NULL) {
        return 0;
    }

    ctx.symbols = new_symtable();
    if (ctx.symbols == NULL) {
        ERROR("semantic error: failed to allocate symbol table");
        return 1;
    }

    check_declarations(&ctx, program->declarations);
    check_statement(&ctx, program->body);

    if (ctx.error_count == 0 && out_symbols != NULL) {
        *out_symbols = ctx.symbols;
        // pass the symbol table to the IR generator so we won't need to rebuild it
    } else {
        // if something failed it's ok to free it (no IR would be generated...)
        free_symtable(ctx.symbols);
    }
    return ctx.error_count;
}
