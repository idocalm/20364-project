#ifndef QUAD_UTILS_H
#define QUAD_UTILS_H

#include "quad.h"

typedef struct
{
    const char *iop;
    const char *rop;
} QuadOpPair;

b8 quad_emitf(QuadContext *ctx, const char *fmt, ...);
const char *quad_new_label(QuadContext *ctx, char *buffer, i32 buffer_len);
void quad_new_temp(QuadContext *ctx, char *buffer, i32 buffer_len);
AstType quad_symbol_type(QuadContext *ctx, const char *name);

QuadValue quad_tmp(QuadContext *ctx, AstType type);
void quad_label(QuadContext *ctx, char out_label[DEFAULT_LABEL_LENGTH]);

QuadValue quad_to_float(QuadContext *ctx, QuadValue v);
QuadValue quad_to_int(QuadContext *ctx, QuadValue v);
QuadValue quad_to_bool_int(QuadContext *ctx, QuadValue v);
QuadValue quad_emit_expr(QuadContext *ctx, const AstExpression *expr);
QuadValue quad_emit_binary(QuadContext *ctx, const AstExpression *expr);

void quad_emit_statement(QuadContext *ctx, const AstStatement *stmt);
void quad_emit_stmt_list(QuadContext *ctx, const AstStatement *stmt);

b8 labels_grow(LabelEntry **label_entry, i32 *cap, i32 min_need);

#endif
