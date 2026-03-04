#include "compiler.h"
#include "ast.h"
#include "parser.h"
#include "debug.h"
#include "semantics.h"
#include "quad.h"

#include <stdio.h>

int compile_file(FILE *in, FILE *out, const char *input_path) {
    AstProgram *program = NULL;
    SymbolTable *symbols = NULL;
    i32 parse_errors = 0;
    i32 semantic_errors = 0;

    if (parser_parse(in, &program, &parse_errors) != 0) {
        ERROR("found %d parsing/lexical error(s) in '%s'", (int)parse_errors, input_path);
        return 1;
    }

    if (DEBUG_PROGRESS)
        debug_print_ast(NULL, program);

    semantic_errors = semantic_check_program(program, &symbols);
    if (semantic_errors > 0) {
        ERROR("found %d semantic error(s) in '%s'", (int)semantic_errors, input_path);
        if (symbols != NULL) {
            free_symtable(symbols);
        }
        free_ast_program(program);
        return 1;
    }

    if (quad_generate(out, program, symbols, input_path) != 0) {
        free_symtable(symbols);
        free_ast_program(program);
        return 1;
    }

    free_symtable(symbols);
    free_ast_program(program);
    return 0;
}
