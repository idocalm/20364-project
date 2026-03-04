#ifndef SEMANTICS_H
#define SEMANTICS_H

#include "ast.h"
#include "defs.h"
#include "symbols.h"

typedef struct semantic_context
{
    SymbolTable *symbols;
    i32 error_count;
} SemanticContext;

i32 semantic_check_program(const AstProgram *program, SymbolTable **out_symbols);

#endif
