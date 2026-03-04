#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#include "ast.h"

void debug_print_ast(FILE *out, const AstProgram *program);

#endif
