#include "compiler.h"
#include "ast.h"
#include "parser.h"
#include "debug.h"

#include <stdio.h>

#define DEBUG 1

int compile_file(FILE *in, FILE *out, const char *input_path) {
    AstProgram *program = NULL;
    i32 parse_errors = 0;

    if (parser_parse(in, &program, &parse_errors) != 0) {
        ERROR("found %d parsing/lexical error(s) in '%s'", (int)parse_errors, input_path);
        return 1;
    }

    if (DEBUG)
        debug_print_ast(NULL, program);

    if (fprintf(out, "HALT\n") < 0)
    {
        ERROR("failed writing output");
        free_ast_program(program);
        return 1;
    }

    free_ast_program(program);
    return 0;
}
