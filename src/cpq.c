#include "compiler.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

i32 main(i32 argc, char **argv) {
    char *input_path;
    char *output_path = NULL;
    char *tmp_output_path = NULL;

    FILE *in = NULL;
    FILE *tmp_out = NULL;
        
    i32 rc = 1;

    LOG(SIGNATURE);

    if (argc != 2) {
        ERROR("usage: cpq <filename>.ou");
        return 1;
    }

    input_path = argv[1];
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

    tmp_output_path = make_temp_output_path(output_path);
    if (tmp_output_path == NULL) {
        ERROR("failed to allocate memory for temporary output path");
        goto cleanup;
    }

    tmp_out = fopen(tmp_output_path, "wb");
    if (tmp_out == NULL) {
        ERROR("failed to open temporary output '%s': %s", tmp_output_path, strerror(errno));
        goto cleanup;
    }

    if (compile_file(in, tmp_out, input_path) != 0) {
        goto cleanup;
    }

    if (fclose(tmp_out) != 0) {
        tmp_out = NULL;
        ERROR("failed to finalize output '%s': %s", tmp_output_path, strerror(errno));
        goto cleanup;
    }
    tmp_out = NULL;

    if (remove(output_path) != 0 && errno != ENOENT) {
        ERROR("failed to replace output '%s': %s", output_path, strerror(errno));
        goto cleanup;
    }

    if (rename(tmp_output_path, output_path) != 0) {
        ERROR("failed to create output '%s': %s", output_path, strerror(errno));
        goto cleanup;
    }

    rc = 0;

cleanup:
    if (tmp_out != NULL)
        fclose(tmp_out);
    if (in != NULL)
        fclose(in);
    if (rc != 0 && tmp_output_path != NULL)
        remove(tmp_output_path);

    free(tmp_output_path);
    free(output_path);

    return rc;
}
