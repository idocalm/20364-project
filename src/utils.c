#include "utils.h"
#include "defs.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

b8 has_ou_suffix(const char *path)
{
    i32 len = strlen(path);
    if (len < 3)
        return false;

    return strcmp(path + len - 3, OU_SUFFIX) == 0;
}

char *make_output_path(const char *input_path)
{
    i32 len = strlen(input_path);
    char *output_path = (char *)malloc(len + 2);
    if (output_path == NULL)
        return NULL;

    memcpy(output_path, input_path, len - 2);
    memcpy(output_path + len - 2, QUAD_SUFFIX, 4);
    return output_path;
}

char *make_temp_output_path(const char *output_path)
{
    char suffix[] = ".tmp";
    i32 out_len = strlen(output_path);
    i32 suffix_len = sizeof(suffix);
    char *tmp_path = (char *)malloc(out_len + suffix_len);
    if (tmp_path == NULL)
        return NULL;

    memcpy(tmp_path, output_path, out_len);
    memcpy(tmp_path + out_len, suffix, suffix_len);
    return tmp_path;
}

b8 is_number(const char *text)
{
    size_t i = 0;
    if (text == NULL || text[0] == '\0')
    {
        return false;
    }
    if (text[0] == '+' || text[0] == '-')
    {
        i = 1;
    }
    
    for (; text[i] != '\0'; i++)
    {
        if (!isdigit((unsigned char)text[i]))
        {
            return false;
        }
    }
    return true;
}

b8 is_char_printable(i8 c)
{
    return c >= 32 && c <= 126;
}
