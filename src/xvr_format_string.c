/**
 * XVR Format String Parser Implementation
 *
 * Parses format strings with {} placeholders
 */

#include "xvr_format_string.h"

#include <stdio.h>
#include <string.h>

#include "xvr_string_utils.h"

#define MAX_PLACEHOLDERS 64

static const char* format_type_to_printf(XvrFormatArgType type) {
    switch (type) {
    case XVR_FORMAT_ARG_INT:
        return "%d";
    case XVR_FORMAT_ARG_FLOAT:
    case XVR_FORMAT_ARG_DOUBLE:
        return "%lf";
    case XVR_FORMAT_ARG_STRING:
        return "%s";
    case XVR_FORMAT_ARG_POINTER:
        return "%p";
    case XVR_FORMAT_ARG_BOOL:
        return "%s";
    case XVR_FORMAT_ARG_ARRAY:
        return ""; /* Arrays handled separately */
    default:
        return "%p";
    }
}

static XvrFormatArgType infer_type_from_format_char(char c) {
    switch (c) {
    case 'd':
    case 'i':
    case 'u':
    case 'o':
    case 'x':
    case 'X':
        return XVR_FORMAT_ARG_INT;
    case 'f':
    case 'F':
    case 'e':
    case 'E':
    case 'g':
    case 'G':
        return XVR_FORMAT_ARG_DOUBLE;
    case 's':
        return XVR_FORMAT_ARG_STRING;
    case 'p':
        return XVR_FORMAT_ARG_POINTER;
    case 'c':
        return XVR_FORMAT_ARG_INT;
    default:
        return XVR_FORMAT_ARG_INT;
    }
}

static void skip_escaped_brace(const char** ptr) {
    const char* p = *ptr;
    if (*p == '{' && *(p + 1) == '{') {
        *ptr = p + 2;
    } else if (*p == '}' && *(p + 1) == '}') {
        *ptr = p + 2;
    }
}

XvrFormatString* XvrFormatStringParse(const char* format_str,
                                      uint32_t arg_count) {
    if (!format_str) {
        return NULL;
    }

    XvrFormatString* fmt = calloc(1, sizeof(XvrFormatString));
    if (!fmt) {
        return NULL;
    }

    fmt->placeholders = calloc(MAX_PLACEHOLDERS, sizeof(XvrFormatPlaceholder));
    if (!fmt->placeholders) {
        free(fmt);
        return NULL;
    }

    size_t fmt_len = xvr_safe_strlen(format_str, 4096);

    /* First pass: count placeholders and detect explicit types */
    uint32_t placeholder_count = 0;
    uint32_t pos = 0;
    const char* p = format_str;

    while (*p && pos < fmt_len) {
        skip_escaped_brace(&p);

        if (*p == '{') {
            /* Found a placeholder */
            const char* start = p + 1;
            char* end = NULL;

            /* Check for empty placeholder or explicit type */
            if (*start == '}') {
                /* Empty {} - infer type from argument position */
                fmt->placeholders[placeholder_count].position =
                    placeholder_count;
                fmt->placeholders[placeholder_count].type =
                    XVR_FORMAT_ARG_INT; /* Default */
                placeholder_count++;
                p = start + 1;
            } else {
                /* Check for format specifier like {:.2f} */
                const char* end = strchr(start, '}');
                if (end) {
                    /* Check for explicit type specifier */
                    if (*start == ':') {
                        /* Has explicit type */
                        const char* spec = start + 1;
                        if (*spec == 'd' || *spec == 'i') {
                            fmt->placeholders[placeholder_count].type =
                                XVR_FORMAT_ARG_INT;
                        } else if (*spec == 'f' || *spec == 'F') {
                            fmt->placeholders[placeholder_count].type =
                                XVR_FORMAT_ARG_DOUBLE;
                        } else if (*spec == 's') {
                            fmt->placeholders[placeholder_count].type =
                                XVR_FORMAT_ARG_STRING;
                        } else if (*spec == 'p') {
                            fmt->placeholders[placeholder_count].type =
                                XVR_FORMAT_ARG_POINTER;
                        } else if (*spec == 'b') {
                            fmt->placeholders[placeholder_count].type =
                                XVR_FORMAT_ARG_BOOL;
                        } else {
                            /* Unknown specifier - infer from spec */
                            fmt->placeholders[placeholder_count].type =
                                infer_type_from_format_char(*spec);
                        }
                    } else {
                        /* No explicit type - check if it's a number
                         * (positional) */
                        char* num_end;
                        long pos_val = strtol(start, &num_end, 10);
                        if (num_end != start && *num_end == '}') {
                            /* Positional placeholder {n} */
                            fmt->placeholders[placeholder_count].position =
                                (uint32_t)pos_val;
                            fmt->placeholders[placeholder_count].type =
                                XVR_FORMAT_ARG_INT;
                        } else {
                            /* Empty placeholder - auto position */
                            fmt->placeholders[placeholder_count].position =
                                placeholder_count;
                            fmt->placeholders[placeholder_count].type =
                                XVR_FORMAT_ARG_INT;
                        }
                    }

                    fmt->placeholders[placeholder_count].position =
                        placeholder_count;
                    placeholder_count++;
                    p = end + 1;
                } else {
                    /* Malformed placeholder */
                    fmt->is_valid = false;
                    fmt->error_message = strdup("Malformed format placeholder");
                    return fmt;
                }
            }
        } else {
            p++;
        }
    }

    fmt->placeholder_count = placeholder_count;
    fmt->arg_count = arg_count;

    /* Validate argument count */
    if (arg_count != placeholder_count) {
        fmt->is_valid = false;
        asprintf(&fmt->error_message,
                 "Format string has %u placeholders but %u arguments provided",
                 placeholder_count, arg_count);
        return fmt;
    }

    /* Second pass: build printf format string */
    size_t result_size = fmt_len * 2 + 1; /* Extra space for % */
    char* result = malloc(result_size);
    if (!result) {
        fmt->is_valid = false;
        return fmt;
    }

    result[0] = '\0';
    placeholder_count = 0;
    p = format_str;
    pos = 0;

    while (*p && pos < fmt_len) {
        /* Handle escaped braces {{ or }} */
        if (p[0] == '{' && p[1] == '{') {
            strcat(result, "{");
            p += 2;
            pos += 2;
            continue;
        }
        if (p[0] == '}' && p[1] == '}') {
            strcat(result, "}");
            p += 2;
            pos += 2;
            continue;
        }

        if (*p == '{') {
            /* Found a placeholder - add printf format specifier */
            const char* start = p + 1;
            const char* end = strchr(start, '}');

            if (end) {
                /* Check for explicit type specifier */
                if (*start == ':') {
                    const char* spec = start + 1;
                    /* Copy the specifier as-is */
                    size_t spec_len = end - spec;
                    char spec_str[32] = {0};
                    if (spec_len < sizeof(spec_str)) {
                        strncpy(spec_str, spec, spec_len);
                        strcat(result, "%");
                        strcat(result, spec_str);
                    }
                } else {
                    /* No explicit type - use placeholder position to determine
                     * type */
                    XvrFormatArgType arg_type =
                        fmt->placeholders[placeholder_count].type;
                    strcat(result, format_type_to_printf(arg_type));
                }
                placeholder_count++;
                p = end + 1;
            } else {
                /* Malformed */
                strcat(result, *p == '{' ? "{" : "}");
                p++;
            }
        } else {
            /* Regular character */
            char tmp[2] = {*p, '\0'};
            strcat(result, tmp);
            p++;
        }
    }

    fmt->printf_format = result;
    fmt->is_valid = true;

    return fmt;
}

void XvrFormatStringFree(XvrFormatString* fmt) {
    if (!fmt) {
        return;
    }
    free(fmt->printf_format);
    free(fmt->placeholders);
    free(fmt->error_message);
    free(fmt);
}

char* XvrFormatStringGetPrintfFormat(const XvrFormatString* fmt) {
    if (!fmt || !fmt->is_valid || !fmt->printf_format) {
        return NULL;
    }
    return strdup(fmt->printf_format);
}

XvrFormatArgType XvrFormatStringGetPlaceholderType(const XvrFormatString* fmt,
                                                   uint32_t index) {
    if (!fmt || !fmt->is_valid || index >= fmt->placeholder_count) {
        return XVR_FORMAT_ARG_INT;
    }
    return fmt->placeholders[index].type;
}

char* XvrFormatStringBuildPrintfFormat(const XvrFormatString* fmt,
                                       XvrFormatArgType* arg_types,
                                       uint32_t arg_count) {
    if (!fmt || !fmt->is_valid) {
        return NULL;
    }

    const char* src = fmt->printf_format;
    if (!src) {
        return strdup("");
    }

    size_t src_len = xvr_safe_strlen(src, 4096);
    size_t result_size = src_len * 2 + 1;
    char* result = malloc(result_size);
    if (!result) return NULL;

    size_t result_pos = 0;
    size_t placeholder_idx = 0;

    for (size_t i = 0; i < src_len; i++) {
        if (src[i] == '%' && i + 1 < src_len) {
            if (src[i + 1] == '%') {
                result[result_pos++] = '%';
                result[result_pos++] = '%';
                i++;
            } else {
                const char* spec_start = &src[i];
                while (i < src_len && src[i] != 'd' && src[i] != 'i' &&
                       src[i] != 'u' && src[i] != 'o' && src[i] != 'x' &&
                       src[i] != 'X' && src[i] != 'f' && src[i] != 'F' &&
                       src[i] != 'e' && src[i] != 'E' && src[i] != 'g' &&
                       src[i] != 'G' && src[i] != 'c' && src[i] != 's' &&
                       src[i] != 'p' && src[i] != 'n') {
                    i++;
                }

                if (i >= src_len ||
                    (src[i] != 'd' && src[i] != 'i' && src[i] != 'u' &&
                     src[i] != 'o' && src[i] != 'x' && src[i] != 'X' &&
                     src[i] != 'f' && src[i] != 'F' && src[i] != 'e' &&
                     src[i] != 'E' && src[i] != 'g' && src[i] != 'G' &&
                     src[i] != 'c' && src[i] != 's' && src[i] != 'p' &&
                     src[i] != 'n')) {
                    result[result_pos++] = '%';
                } else if (placeholder_idx < arg_count &&
                           arg_types[placeholder_idx] != XVR_FORMAT_ARG_INT) {
                    const char* replacement =
                        format_type_to_printf(arg_types[placeholder_idx]);
                    size_t repl_len = xvr_safe_strlen(replacement, 32);
                    memcpy(&result[result_pos], replacement, repl_len);
                    result_pos += repl_len;
                } else {
                    size_t spec_len = &src[i] - spec_start + 1;
                    memcpy(&result[result_pos], spec_start, spec_len);
                    result_pos += spec_len;
                }
                placeholder_idx++;
            }
        } else {
            result[result_pos++] = src[i];
        }
    }
    result[result_pos] = '\0';
    return result;
}

bool XvrFormatStringHasPlaceholders(const char* str) {
    if (!str) {
        return false;
    }

    while (*str) {
        if (*str == '{' && *(str + 1) != '{') {
            /* Found unescaped { */
            return true;
        }
        str++;
    }
    return false;
}
