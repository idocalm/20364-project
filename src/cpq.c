#include "compiler.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

b8 debug_progress = false;

static b8 parse_args(i32 argc, char **argv, char **out_input_path) {
    i32 i = 0;
    char *input_path = NULL;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug_progress = true;
            continue;
        }

        if (argv[i][0] == '-') {
            return false;
        }

        if (input_path != NULL) {
            return false;
        }
        input_path = argv[i];
    }

    *out_input_path = input_path;
    return input_path != NULL;
}

i32 main(i32 argc, char **argv) {
    char *input_path;
    char *output_path = NULL;

    FILE *in = NULL;
    FILE *out = NULL;

    i32 rc = 1;

    LOG(SIGNATURE);

    if (!parse_args(argc, argv, &input_path)) {
        ERROR("usage: cpq [--debug] <filename>.ou");
        return 1;
    }
    LOG("DEBUG MODE [--debug]: %s", debug_progress ? "ENABLED" : "DISABLED");

    if (!has_ou_suffix(input_path)) {
        ERROR("input file must have .ou suffix");
        return 1;
    }

    in = fopen(input_path, "rb");
    if (in == NULL) {
        ERROR("failed to open input '%s': %s", input_path, strerror(errno));
        goto cleanup;
    }

    output_path = make_output_path(input_path);
    if (output_path == NULL) {
        ERROR("failed to allocate memory for output path");
        goto cleanup;
    }

    out = fopen(output_path, "wb");
    if (out == NULL) {
        ERROR("failed to open output '%s': %s", output_path, strerror(errno));
        goto cleanup;
    }

    if (compile_file(in, out, input_path) != 0) {
        goto cleanup;
    }

    if (fclose(out) != 0) {
        out = NULL;
        ERROR("failed to finalize output '%s': %s", output_path, strerror(errno));
        goto cleanup;
    }
    out = NULL;
    LOG("success: generated '%s'", output_path);

    rc = 0;

cleanup:
    if (out != NULL)
        fclose(out);
    if (in != NULL)
        fclose(in);
    if (rc != 0 && output_path != NULL)
        remove(output_path);

    free(output_path);

    return rc;
}
