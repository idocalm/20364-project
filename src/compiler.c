#include "compiler.h"
#include "lexer.h"

#include <stdio.h>

int compile_file(FILE *in, FILE *out, const char *input_path) {
    i32 token = TOKEN_EOF;
    i32 lex_errors = 0;

    if (!lexer_begin(in)) {
        ERROR("failed to initialize lexer");
        return 1;
    }

    do {
        token = lexer_next();
    } while (token != TOKEN_EOF);

    lex_errors = lexer_error_count();
    lexer_end();
    if (lex_errors > 0) {
        ERROR("found %d lexical error(s) in '%s'", (i32)lex_errors, input_path);
        return 1;
    }

    return 0;
}

