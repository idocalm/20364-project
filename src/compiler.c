#include "compiler.h"
#include "ast.h"
#include "parser.h"
#include "debug.h"
#include "semantics.h"

#include <stdio.h>

#define DEBUG_AST 1

int compile_file(FILE *in, FILE *out, const char *input_path) {
    AstProgram *program = NULL;
    i32 parse_errors = 0;
    i32 semantic_errors = 0;

    if (parser_parse(in, &program, &parse_errors) != 0) {
        ERROR("found %d parsing/lexical error(s) in '%s'", (int)parse_errors, input_path);
        return 1;
    }

    if (DEBUG_AST)
        debug_print_ast(NULL, program);

    semantic_errors = semantic_check_program(program);
    if (semantic_errors > 0) {
        ERROR("found %d semantic error(s) in '%s'", (int)semantic_errors, input_path);
        free_ast_program(program);
        return 1;
    }

    if (fprintf(out, "HALT\n") < 0)
    {
        ERROR("failed writing output");
        free_ast_program(program);
        return 1;
    }

    free_ast_program(program);
    return 0;
}
