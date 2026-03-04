#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

#include "defs.h"
#include "utils.h"

b8 lexer_begin(FILE *input_file);
void lexer_end(void);
i32 lexer_next(void);
i32 lexer_line(void);
i32 lexer_column(void);
const char *lexer_text(void);
const char *lexer_line_preview(void);
i32 lexer_error_count(void);

static inline void lexer_capture_preview(
    char *line_preview,
    i32 *line_preview_len,
    i32 line_preview_cap,
    const char *text,
    i32 text_len
) {
    i32 i = 0;
    for (i = 0; i < text_len; i++) {
        char c = text[i];
        if (c == '\n') {
            *line_preview_len = 0;
            continue;
        }

        if (*line_preview_len < line_preview_cap - 1) {
            line_preview[(*line_preview_len)++] = c;
            line_preview[*line_preview_len] = '\0';
        }
    }
}

static inline void lexer_print_caret_line(i32 column, i32 width) {
    i32 i = 0;
    fprintf(stderr, "    ");
    for (i = 1; i < column; i++) {
        fputc(' ', stderr);
    }
    for (i = 0; i < width; i++) {
        fputc('^', stderr);
    }
    fputc('\n', stderr);
}

#endif

