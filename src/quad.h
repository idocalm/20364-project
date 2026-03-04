#ifndef QUAD_H
#define QUAD_H

#include <stdio.h>

#include "ast.h"
#include "defs.h"
#include "symbols.h"

#define DEFAULT_LABEL_LENGTH 64
#define MAX_BREAK_DEPTH 128
#define INITIAL_LABELS_CAP 10

typedef struct quad_value
{
    AstType type;
    char place[DEFAULT_LABEL_LENGTH];
} QuadValue;

typedef struct quad_ctx
{
    FILE *out;            // file we are writing to
    SymbolTable *symbols; // symbol table for lookups
    i32 temp_counter;     // how many variables were defined up to now
    i32 label_counter;    // how many labels were defined up to now
    const char *break_labels[MAX_BREAK_DEPTH]; // break exit labels array 
    i32 break_depth; // max break depth
    i32 error; // did get into an error?
} QuadContext;

// NOTE: It's possible that some errors would be detected only when generating the IR>
// This would happen for break outside of while for example

// each label we will use in the first stage is stored as a LabelEntry
typedef struct label_entry
{
    char name[DEFAULT_LABEL_LENGTH];
    i32 line;
} LabelEntry;

// create the first stage Quad code with labels, return 0 on success
i32 quad_first_stage(FILE *tmp_out, const AstProgram *program, SymbolTable *symbols);

// finalize jumps and remove the labels, generating the final Quad code
i32 quad_finalize(FILE *tmp_in, FILE *final_out);

i32 quad_generate(FILE *final_out, const AstProgram *program, SymbolTable *symbols, const char *input_path);

QuadValue quad_to_bool_int(QuadContext *ctx, QuadValue v);

#endif
