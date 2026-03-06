#include "quad.h"
#include "quad_utils.h"

#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "symbols.h"
#include "utils.h"

// safely write a line to the quad file
static b8 quad_emit_line(FILE *out, const char *fmt, ...)
{
    if (!out)
        return false;

    va_list args;
    va_start(args, fmt);
    int res = vfprintf(out, fmt, args);
    va_end(args);

    return res >= 0;
}

// emit a new label L{i}, where i is the updated label counter
const char *quad_new_label(QuadContext *ctx, char *buffer, i32 buffer_len)
{
    ctx->label_counter++;
    snprintf(buffer, (size_t)buffer_len, "L%d", (int)ctx->label_counter);
    return buffer;
}

// emit a new temp variable t{i}, where i is the updated variable counter
void quad_new_temp(QuadContext *ctx, char *buffer, i32 buffer_len)
{
    ctx->temp_counter++;
    snprintf(buffer, (size_t)buffer_len, "_t%d", (int)ctx->temp_counter);
}

b8 quad_emitf(QuadContext *ctx, const char *fmt, ...)
{
    va_list args;
    if (ctx->error)
    {
        return false;
    }
    va_start(args, fmt);
    if (vfprintf(ctx->out, fmt, args) < 0)
    {
        va_end(args);
        ctx->error = 1;
        return false;
    }
    va_end(args);
    return true;
}

// utility function for getting the AstType (int, float) of a symbol 
AstType quad_symbol_type(QuadContext *ctx, const char *name)
{
    SymbolEntry *entry = symtable_find(ctx->symbols, name);
    if (entry == NULL)
    {
        ERROR("quad error: unknown identifier '%s'", name);
        ctx->error = 1;
        return AST_TYPE_INT;
    }
    return entry->type;
}

QuadValue quad_emit_expr(QuadContext *ctx, const AstExpression *expr);

// cast to float (ITOR D B)
QuadValue quad_to_float(QuadContext *ctx, QuadValue v)
{
    QuadValue out = v;
    if (ctx->error || v.type == AST_TYPE_FLOAT)
    {
        // if the type is already float, don't use ITOR on it
        return out;
    }
    out.type = AST_TYPE_FLOAT; // change the type
    quad_new_temp(ctx, out.place, (i32)sizeof(out.place));
    quad_emitf(ctx, "ITOR %s %s\n", out.place, v.place);
    return out;
}

// cast to int (RTOI A E)
QuadValue quad_to_int(QuadContext *ctx, QuadValue v)
{
    QuadValue out = v;
    if (ctx->error || v.type == AST_TYPE_INT)
    {
        // if the type is already int, don't use RTOI on it
        return out;
    }
    out.type = AST_TYPE_INT;
    quad_new_temp(ctx, out.place, (i32)sizeof(out.place));
    quad_emitf(ctx, "RTOI %s %s\n", out.place, v.place);
    return out;
}

// Interepert a int / float as a bool 
QuadValue quad_to_bool_int(QuadContext *ctx, QuadValue v)
{
    QuadValue out;
    if (v.type == AST_TYPE_INT)
    {
        // if it's a int, it can already be considered a boolean
        return v;
    }

    // otherwise return as an int the result of v != 0.0 with RNQL
    out.type = AST_TYPE_INT;
    quad_new_temp(ctx, out.place, (i32)sizeof(out.place));
    quad_emitf(ctx, "RNQL %s %s 0.0\n", out.place, v.place);
    return out;
}

// emit any kind of expression
QuadValue quad_emit_expr(QuadContext *ctx, const AstExpression *expr)
{
    QuadValue out;
    out.type = AST_TYPE_INT;
    out.place[0] = '\0';

    if (expr == NULL)
    {
        ERROR("quad error: null expression");
        ctx->error = 1;
        return out;
    }

    switch (expr->kind)
    {
        case AST_EXPR_ID:
            // ids can be written as they are
            out.type = quad_symbol_type(ctx, expr->as.identifier);
            snprintf(out.place, sizeof(out.place), "%s", expr->as.identifier);
            return out;

        case AST_EXPR_INT_LITERAL:
            // literals can be written as they are 
            out.type = AST_TYPE_INT;
            snprintf(out.place, sizeof(out.place), "%s", expr->as.number_text);
            return out;

        case AST_EXPR_FLOAT_LITERAL:
            // literals can be written as they are
            out.type = AST_TYPE_FLOAT;
            snprintf(out.place, sizeof(out.place), "%s", expr->as.number_text);
            return out;

        case AST_EXPR_CAST_INT:
            // cast<<int>>
            return quad_to_int(ctx, quad_emit_expr(ctx, expr->as.unary.operand));

        case AST_EXPR_CAST_FLOAT:
            // cast<<float>>
            return quad_to_float(ctx, quad_emit_expr(ctx, expr->as.unary.operand));

        case AST_EXPR_NOT:
        {
            QuadValue v = quad_emit_expr(ctx, expr->as.unary.operand);
            QuadValue t = quad_to_bool_int(ctx, v);
            out.type = AST_TYPE_INT;
            quad_new_temp(ctx, out.place, (i32)sizeof(out.place));
            quad_emitf(ctx, "IEQL %s %s 0\n", out.place, t.place);
            return out;
        }

        case AST_EXPR_BINARY:
            return quad_emit_binary(ctx, expr);

        default:
            ERROR("quad error: unsupported expression kind");
            ctx->error = 1;
            return out;
    }
}

void quad_emit_stmt_list(QuadContext *ctx, const AstStatement *stmt)
{
    // emit each statement in the list by itself
    while (stmt != NULL && !ctx->error)
    {
        quad_emit_statement(ctx, stmt);
        stmt = stmt->next;
    }
}

QuadValue quad_tmp(QuadContext *ctx, AstType type)
{
    QuadValue out;
    out.type = type;
    out.place[0] = '\0';
    quad_new_temp(ctx, out.place, (i32)sizeof(out.place));
    return out;
}

void quad_label(QuadContext *ctx, char out_label[DEFAULT_LABEL_LENGTH])
{
    (void)quad_new_label(ctx, out_label, DEFAULT_LABEL_LENGTH);
}

i32 quad_first_stage(FILE *tmp_out, const AstProgram *program, SymbolTable *symbols)
{
    // first stage contains creating the whole Quad code, except for jump instructions
    // We still use labels, which will be replaced with line numbers in finalizing

    QuadContext ctx;
    if (tmp_out == NULL || program == NULL || symbols == NULL)
    {
        return 1;
    }

    memset(&ctx, 0, sizeof(ctx));
    ctx.out = tmp_out;
    ctx.symbols = symbols;

    quad_emit_statement(&ctx, program->body);
    if (!ctx.error)
    {
        // Add a single HALT instruction at the end of a valid program
        // also sign
        quad_emitf(&ctx, "HALT\n");
        quad_emitf(&ctx, SIGNATURE);
    }

    return ctx.error ? 1 : 0;
}


static i32 quad_find_label(LabelEntry *labels, i32 count, const char *name)
{
    i32 i = 0;
    for (i = 0; i < count; i++)
    {
        if (strcmp(labels[i].name, name) == 0)
        {
            return labels[i].line;
        }
    }
    return -1;
}

i32 quad_finalize(FILE *tmp_in, FILE *final_out)
{
    char line[MAX_LINE_LENGTH];
    
    LabelEntry *labels = NULL;
    i32 cap = INITIAL_LABELS_CAP;
    i32 count = 0;
    i32 current_line = 0;

    if (tmp_in == NULL || final_out == NULL)
    {
        return 1;
    }

    labels = (LabelEntry *) calloc((size_t) cap, sizeof(LabelEntry));
    if (labels == NULL)
    {
        ERROR("quad error: out of memory");
        return 1;
    }

    // pass 1: build a mapping of label to the instruction line
    rewind(tmp_in);
    while (fgets(line, (int)sizeof(line), tmp_in) != NULL)
    {
        char op[32] = {0};
        char arg1[64] = {0};
        if (sscanf(line, "%31s %63s", op, arg1) < 1)
        {
            continue;
        }

        if (strcmp(op, "LABEL") == 0)
        {
            if (!labels_grow(&labels, &cap, count + 1))
            {
                ERROR("quad error: out of memory");
                free(labels);
                return 1;
            }

            snprintf(labels[count].name, sizeof(labels[count].name), "%s", arg1);
            labels[count].line = current_line + 1; // we start with line 1 and not 0 !
            count++;
        }
        else
        {
            current_line++;
        }
    }

    // pass 2: resolve labels in JUMP and JUMPZ instructions
    rewind(tmp_in);
    while (fgets(line, (int)sizeof(line), tmp_in) != NULL)
    {
        char op[32] = {0};
        if (sscanf(line, "%31s", op) < 1)
        {
            continue;
        }

        if (strcmp(op, "LABEL") == 0)
        {
            // Do not copy the previous LABEL declarations
            continue;
        }

        if (strcmp(op, "JUMP") == 0)
        {
            char target[DEFAULT_LABEL_LENGTH] = {0};
            if (sscanf(line, "%31s %63s", op, target) == 2 && !is_number(target))
            {
                i32 target_line = quad_find_label(labels, count, target);
                if (target_line < 1)
                {
                    ERROR("quad error: unresolved label '%s'", target);
                    goto fail;
                }
                // replace JUMP label with JUMP line
                if (!quad_emit_line(final_out, "JUMP %d\n", (int)target_line))
                {
                    goto fail;
                }
                continue;
            }
        }

        if (strcmp(op, "JMPZ") == 0)
        {
            char target[DEFAULT_LABEL_LENGTH] = {0}, cond[DEFAULT_LABEL_LENGTH] = {0};
            if (sscanf(line, "%31s %63s %63s", op, target, cond) == 3 && !is_number(target))
            {
                i32 target_line = quad_find_label(labels, count, target);
                if (target_line < 1)
                {
                    ERROR("quad error: unresolved label '%s'", target);
                    goto fail;
                }
                if (!quad_emit_line(final_out, "JMPZ %d %s\n", (int)target_line, cond))
                {
                    goto fail;
                }
                continue;
            }
        }

        if (!quad_emit_line(final_out, "%s", line)) // keep other code as is
        {
            goto fail;
        }
    }

    free(labels);
    return 0;
fail:
    free(labels);
    return 0;
}

i32 quad_generate(FILE *final_out, const AstProgram *program, SymbolTable *symbols, const char *input_path)
{
    char *output_path = NULL;
    char *tmp_path = NULL;
    FILE *tmp_file = NULL;
    i32 rc = 1;

    if (final_out == NULL || program == NULL || symbols == NULL || input_path == NULL)
    {
        ERROR("quad error: invalid arguments");
        return 1;
    }

    output_path = make_output_path(input_path);
    if (output_path == NULL)
    {
        ERROR("quad error: failed to allocate output path");
        goto cleanup;
    }

    tmp_path = make_temp_output_path(output_path);
    if (tmp_path == NULL)
    {
        ERROR("quad error: failed to allocate temp output path");
        goto cleanup;
    }

    tmp_file = fopen(tmp_path, "wb+");
    if (tmp_file == NULL)
    {
        ERROR("quad error: failed to open temp file '%s': %s", tmp_path, strerror(errno));
        goto cleanup;
    }

    if (quad_first_stage(tmp_file, program, symbols) != 0)
    {
        goto cleanup;
    }

    if (quad_finalize(tmp_file, final_out) != 0)
    {
        goto cleanup;
    }

    rc = 0;

cleanup:
    if (tmp_file != NULL)
    {
        fclose(tmp_file);
    }

    if (tmp_path != NULL && !debug_progress)
    {
        remove(tmp_path);
    }

    free(tmp_path);
    free(output_path);
    return rc;
}
