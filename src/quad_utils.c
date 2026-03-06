#include "quad.h"
#include "quad_utils.h"
#include "defs.h"
#include "log.h"
#include <stdlib.h>

///////////////// for emitting a general binary operation /////////////////

static inline b8 quad_is_float(QuadValue a, QuadValue b)
{
    return a.type == AST_TYPE_FLOAT || b.type == AST_TYPE_FLOAT;
}

static inline QuadValue quad_emit_arith(QuadContext *ctx, QuadOpPair op, QuadValue l, QuadValue r, b8 is_float)
{
    QuadValue out = quad_tmp(ctx, is_float ? AST_TYPE_FLOAT : AST_TYPE_INT);
    if (is_float)
    {
        l = quad_to_float(ctx, l);
        r = quad_to_float(ctx, r);

        quad_emitf(ctx, "%s %s %s %s\n", op.rop, out.place, l.place, r.place);
    }
    else
    {
        quad_emitf(ctx, "%s %s %s %s\n", op.iop, out.place, l.place, r.place);
    }

    return out;
}

static inline QuadValue quad_emit_cmp(QuadContext *ctx, QuadOpPair op, QuadValue l, QuadValue r, b8 is_float)
{
    QuadValue out = quad_tmp(ctx, AST_TYPE_INT);
    if (is_float)
    {
        l = quad_to_float(ctx, l);
        r = quad_to_float(ctx, r);
        quad_emitf(ctx, "%s %s %s %s\n", op.rop, out.place, l.place, r.place);
    }
    else
    {
        quad_emitf(ctx, "%s %s %s %s\n", op.iop, out.place, l.place, r.place);
    }
    return out;
}

static inline QuadValue quad_invert_bool(QuadContext *ctx, QuadValue b)
{
    QuadValue inv = quad_tmp(ctx, AST_TYPE_INT);
    quad_emitf(ctx, "IEQL %s %s 0\n", inv.place, b.place);
    return inv;
}

QuadValue quad_emit_binary(QuadContext *ctx, const AstExpression *expr)
{
    QuadValue out = {
        .type = AST_TYPE_INT,
        .place = {0}};
    QuadValue left = quad_emit_expr(ctx, expr->as.binary.left);
    QuadValue right = quad_emit_expr(ctx, expr->as.binary.right);

    if (ctx->error)
        return out;

    const b8 is_float = quad_is_float(left, right);

    switch (expr->as.binary.op)
    {

    // we always use quad_emit_arith with a pair of opcodes when we need to differ floats and ints. If neeeded, this function will covert

    // arithmetic
    case AST_BINOP_ADD:
        return quad_emit_arith(ctx, (QuadOpPair){"IADD", "RADD"}, left, right, is_float);
    case AST_BINOP_SUB:
        return quad_emit_arith(ctx, (QuadOpPair){"ISUB", "RSUB"}, left, right, is_float);
    case AST_BINOP_MUL:
        return quad_emit_arith(ctx, (QuadOpPair){"IMLT", "RMLT"}, left, right, is_float);
    case AST_BINOP_DIV:
        return quad_emit_arith(ctx, (QuadOpPair){"IDIV", "RDIV"}, left, right, is_float);

    // comparisons (return int representing a bool)
    case AST_BINOP_EQ:
        return quad_emit_cmp(ctx, (QuadOpPair){"IEQL", "REQL"}, left, right, is_float);
    case AST_BINOP_NE:
        return quad_emit_cmp(ctx, (QuadOpPair){"INQL", "RNQL"}, left, right, is_float);
    case AST_BINOP_LT:
        return quad_emit_cmp(ctx, (QuadOpPair){"ILSS", "RLSS"}, left, right, is_float);
    case AST_BINOP_GT:
        return quad_emit_cmp(ctx, (QuadOpPair){"IGRT", "RGRT"}, left, right, is_float);

    case AST_BINOP_GE:
        // x >= y if and only if NOT x < y
        return quad_invert_bool(ctx, quad_emit_cmp(ctx, (QuadOpPair){"ILSS", "RLSS"}, left, right, is_float));
    case AST_BINOP_LE:
        // x <= y if and only if NOT x > y
        return quad_invert_bool(ctx, quad_emit_cmp(ctx, (QuadOpPair){"IGRT", "RGRT"}, left, right, is_float));

    case AST_BINOP_OR:
    {
        // To perform a || b:
        // calculate trueness value of a, b
        // sum = a + b --> IADD sum a b
        // IGRT result s 0
        // then result == 0 iff a = b = 0

        QuadValue l = quad_to_bool_int(ctx, left);
        QuadValue r = quad_to_bool_int(ctx, right);
        QuadValue sum = quad_tmp(ctx, AST_TYPE_INT);
        quad_emitf(ctx, "IADD %s %s %s\n", sum.place, l.place, r.place);
        QuadValue b = quad_tmp(ctx, AST_TYPE_INT);
        quad_emitf(ctx, "IGRT %s %s 0\n", b.place, sum.place);
        return b;
    }

    case AST_BINOP_AND:
    {
        // to perform && between x and y
        // calculate x * y (IMLT)
        // x * y = 0 iff x = 0 and/or y = 0

        QuadValue l = quad_to_bool_int(ctx, left);
        QuadValue r = quad_to_bool_int(ctx, right);
        QuadValue b = quad_tmp(ctx, AST_TYPE_INT);
        quad_emitf(ctx, "IMLT %s %s %s\n", b.place, l.place, r.place);
        return b;
    }

    default:
        ERROR("quad error: unsupported binary op");
        ctx->error = 1;
        return out;
    }
}

///////////////// for emitting a general statement /////////////////

// Push a break exit label to the break_labels array
// this is used so we can support nested switch cases and jump to this level exit label
static b8 quad_push_break(QuadContext *ctx, const char *label)
{
    if (ctx->break_depth >= MAX_BREAK_DEPTH)
    {
        ERROR("quad error: break label stack overflow");
        ctx->error = 1;
        return false;
    }
    ctx->break_labels[ctx->break_depth++] = label;
    return true;
}

// "pop" a break exit label
// the array is statically allocated to MAX_BREAK_DEPTH, so we just decrease the counter
// the next time a break label will be pushed it will overwrite the array cell..
static void quad_pop_break(QuadContext *ctx)
{
    if (ctx->break_depth > 0)
    {
        ctx->break_depth--;
    }
}

// Get the current level break label we're expecting to jump to on break
static const char *quad_current_break(const QuadContext *ctx)
{
    if (ctx->break_depth <= 0)
    {
        return NULL;
    }
    return ctx->break_labels[ctx->break_depth - 1];
}

static inline const char *op_by_type(AstType t, const char *iop, const char *rop)
{
    return (t == AST_TYPE_INT) ? iop : rop;
}

static inline void quad_emit_typed2(QuadContext *ctx, AstType t,
                                    const char *iop, const char *rop,
                                    const char *a, const char *b)
{
    quad_emitf(ctx, "%s %s %s\n", op_by_type(t, iop, rop), a, b);
}

static inline void quad_emit_typed1(QuadContext *ctx, AstType t,
                                    const char *iop, const char *rop,
                                    const char *a)
{
    quad_emitf(ctx, "%s %s\n", op_by_type(t, iop, rop), a);
}

static inline QuadValue quad_emit_truthy(QuadContext *ctx, const AstExpression *e)
{
    QuadValue v = quad_emit_expr(ctx, e);
    return quad_to_bool_int(ctx, v);
}

void quad_emit_statement(QuadContext *ctx, const AstStatement *stmt)
{
    if (!stmt || ctx->error)
        return;

    switch (stmt->kind)
    {
    case AST_STMT_ASSIGN:
    {
        const char *id = stmt->as.assign_stmt.identifier;
        AstType dst = quad_symbol_type(ctx, id);
        QuadValue src = quad_emit_expr(ctx, stmt->as.assign_stmt.expression);
        if (ctx->error)
            return;

        if (dst == AST_TYPE_FLOAT && src.type == AST_TYPE_INT)
            src = quad_to_float(ctx, src);

        quad_emit_typed2(ctx, dst, "IASN", "RASN", id, src.place);
        return;
    }

    case AST_STMT_INPUT:
    {
        const char *id = stmt->as.input_stmt.identifier;
        AstType t = quad_symbol_type(ctx, id);
        quad_emit_typed1(ctx, t, "IINP", "RINP", id);
        return;
    }

    case AST_STMT_OUTPUT:
    {
        QuadValue v = quad_emit_expr(ctx, stmt->as.output_stmt.expression);
        quad_emit_typed1(ctx, v.type, "IPRT", "RPRT", v.place);
        return;
    }

    case AST_STMT_IF:
    {
        // for an if statement with an else clause, we need 2 labels:
        // evaluate the condition -> c (0 / 1)
        // JMPZ L_else c
        // code for then stmt
        // JUMP L_end
        // LABEL L_else
        // code for else stmt
        // LABEL L_end

        // for if without else:
        // evaluate the condition -> c (0 / 1)
        // JMPZ L_else c
        // code for then stmt
        // LABEL L_else

        char l_else[DEFAULT_LABEL_LENGTH], l_end[DEFAULT_LABEL_LENGTH];
        quad_new_label(ctx, l_else, DEFAULT_LABEL_LENGTH);
        quad_new_label(ctx, l_end, DEFAULT_LABEL_LENGTH);

        QuadValue c = quad_emit_truthy(ctx, stmt->as.if_stmt.condition);
        quad_emitf(ctx, "JMPZ %s %s\n", l_else, c.place);

        quad_emit_statement(ctx, stmt->as.if_stmt.then_stmt);

        if (stmt->as.if_stmt.else_stmt)
            quad_emitf(ctx, "JUMP %s\n", l_end);
        quad_emitf(ctx, "LABEL %s\n", l_else);

        if (stmt->as.if_stmt.else_stmt)
        {
            quad_emit_statement(ctx, stmt->as.if_stmt.else_stmt);
            quad_emitf(ctx, "LABEL %s\n", l_end);
        }

        return;
    }

    case AST_STMT_WHILE:
    {
        // for while:
        // LABEL L_start
        // evaulate condition -> c (0/1)
        // JMPZ L_end c
        // body code
        // JUMP L_start
        // LABEL L_end

        char l_start[DEFAULT_LABEL_LENGTH], l_end[DEFAULT_LABEL_LENGTH];
        quad_label(ctx, l_start);
        quad_label(ctx, l_end);

        quad_emitf(ctx, "LABEL %s\n", l_start);

        QuadValue c = quad_emit_truthy(ctx, stmt->as.while_stmt.condition);
        quad_emitf(ctx, "JMPZ %s %s\n", l_end, c.place);

        quad_push_break(ctx, l_end);
        quad_emit_statement(ctx, stmt->as.while_stmt.body);
        quad_pop_break(ctx);

        quad_emitf(ctx, "JUMP %s\n", l_start);
        quad_emitf(ctx, "LABEL %s\n", l_end);
        return;
    }

    case AST_STMT_SWITCH:
    {
        /*
            if we have:
            switch (expr) {
                case k1: s1;
                case k2: s2;
                ...
                default: sd
            }

            we need to run all cases until a break
        */

        // compute switch value -> sv
        // cast sv to an int, svi

        // IEQL t_cmp svi k1
        // JMPZ L_next1 t_cmp
        // code for s1
        // LABEL L_next1

        // IEQL t_cmp svi k2
        // JMPZ L_next2 t_cmp
        // code for s2
        // LABEL L_next2

        // ...

        // code for default
        // LABEL L_end

        // a break will add its own code to jump to this levels L_end (!)

        char l_end[64];
        quad_label(ctx, l_end);

        QuadValue sv = quad_emit_expr(ctx, stmt->as.switch_stmt.expression);
        QuadValue svi = quad_to_int(ctx, sv);

        quad_push_break(ctx, l_end);

        for (const AstCase *c = stmt->as.switch_stmt.cases; c && !ctx->error; c = c->next)
        {
            char l_next[DEFAULT_LABEL_LENGTH];
            quad_label(ctx, l_next);

            QuadValue cmp = {.type = AST_TYPE_INT};
            quad_new_temp(ctx, cmp.place, (i32)sizeof(cmp.place));
            quad_emitf(ctx, "IEQL %s %s %s\n", cmp.place, svi.place, c->int_literal);
            quad_emitf(ctx, "JMPZ %s %s\n", l_next, cmp.place);

            quad_emit_stmt_list(ctx, c->statements);
            quad_emitf(ctx, "LABEL %s\n", l_next);
        }

        quad_emit_stmt_list(ctx, stmt->as.switch_stmt.default_statements);
        quad_emitf(ctx, "LABEL %s\n", l_end);
        quad_pop_break(ctx);
        return;
    }

    case AST_STMT_BREAK:
    {
        const char *jumpto_label = quad_current_break(ctx);
        // the jumpto_label was created by the while statement
        if (!jumpto_label)
        {
            ERROR("quad error: break used outside loop/switch");
            ctx->error = 1;
            return;
        }
        quad_emitf(ctx, "JUMP %s\n", jumpto_label);
        return;
    }

    case AST_STMT_BLOCK:
        quad_emit_stmt_list(ctx, stmt->as.block_stmt.statements);
        return;

    default:
        ERROR("quad error: unsupported statement kind");
        ctx->error = 1;
        return;
    }
}

// when finalizing, we don't know how many labels we will encounter
// in my opinion, it would be bad to hard limit this like we did in the max break nesting stack
// instead, we dynmically double the label entry (with realloc) when there's no more space
// doubling also allows us to do realloc-ations not too often    
b8 labels_grow(LabelEntry **label_entry, i32 *cap, i32 min_need)
{
    if (min_need <= *cap)
        return true;

    i32 new_cap = *cap;
    while (new_cap < min_need)
        new_cap *= 2;

    LabelEntry *new_label_entry = (LabelEntry *) realloc(*label_entry, (size_t)new_cap * sizeof(LabelEntry));
    if (!new_label_entry) return false;

    *label_entry = new_label_entry;
    *cap = new_cap;

    return true;
}
