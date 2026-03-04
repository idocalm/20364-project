#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "ast.h"
#include "defs.h"

i32 parser_parse(FILE *input_file, AstProgram **out_program, i32 *out_error_count);

#endif
