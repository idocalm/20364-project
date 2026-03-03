#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>
#include "utils.h"
#include "log.h"

/*
 * Frontend pipeline entrypoint.
 * Returns 0 on success, non-zero when compilation errors were found.
 */
int compile_file(FILE *in, FILE *out, const char *input_path);

#endif
