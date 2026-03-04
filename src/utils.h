#ifndef UTILS_H
#define UTILS_H

#include "defs.h"

b8 has_ou_suffix(const char *path);
char *make_output_path(const char *input_path);
char *make_temp_output_path(const char *output_path);
b8 is_char_printable(i8 c);
b8 is_number(const char *text);

#endif // UTILS_H
