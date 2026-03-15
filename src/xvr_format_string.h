/**
 * XVR Format String Parser
 *
 * Parses format strings with {} placeholders
 * and converts them to printf format strings for code generation.
 *
 * Security: User-controlled strings are NEVER interpreted as format strings.
 * Only literal format strings passed to print() are parsed.
 */

#ifndef XVR_FORMAT_STRING_H
#define XVR_FORMAT_STRING_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* Format argument types */
typedef enum {
    XVR_FORMAT_ARG_INT,
    XVR_FORMAT_ARG_FLOAT,
    XVR_FORMAT_ARG_DOUBLE,
    XVR_FORMAT_ARG_STRING,
    XVR_FORMAT_ARG_POINTER,
    XVR_FORMAT_ARG_BOOL,
    XVR_FORMAT_ARG_ARRAY,
} XvrFormatArgType;

/* A single format placeholder */
typedef struct {
    uint32_t position;     /* Position in format string (0-indexed) */
    XvrFormatArgType type; /* Expected argument type */
    bool has_precision;    /* For floats */
    int precision;
} XvrFormatPlaceholder;

/* Parsed format string result */
typedef struct {
    char* printf_format; /* Converted printf format string */
    XvrFormatPlaceholder* placeholders;
    uint32_t placeholder_count;
    uint32_t arg_count;  /* Number of arguments expected */
    bool is_valid;       /* Whether parsing was successful */
    char* error_message; /* Error message if parsing failed */
} XvrFormatString;

/**
 * @brief Parse a format string with {} placeholders
 *
 * This function ONLY parses string LITERALS. User-controlled strings
 * passed to print() will NOT be interpreted as format strings.
 *
 * @param format_str The format string literal (e.g., "value = {}")
 * @param arg_count Number of arguments provided
 * @return Parsed format string object, or NULL on error
 */
XvrFormatString* XvrFormatStringParse(const char* format_str,
                                      uint32_t arg_count);

/**
 * @brief Free a parsed format string
 * @param fmt Parsed format string to free
 */
void XvrFormatStringFree(XvrFormatString* fmt);

/**
 * @brief Get the printf format string for LLVM IR generation
 * @param fmt Parsed format string
 * @return printf-compatible format string (caller must free)
 */
char* XvrFormatStringGetPrintfFormat(const XvrFormatString* fmt);

/**
 * @brief Get the type of a specific placeholder
 * @param fmt Parsed format string
 * @param index Placeholder index (0-based)
 * @return The expected type for that placeholder, or XVR_FORMAT_ARG_INT if
 * invalid index
 */
XvrFormatArgType XvrFormatStringGetPlaceholderType(const XvrFormatString* fmt,
                                                   uint32_t index);

/**
 * @brief Build printf format string with actual argument types
 * @param fmt Parsed format string
 * @param arg_types Array of argument types (XvrFormatArgType)
 * @param arg_count Number of arguments
 * @return printf-compatible format string (caller must free)
 */
char* XvrFormatStringBuildPrintfFormat(const XvrFormatString* fmt,
                                       XvrFormatArgType* arg_types,
                                       uint32_t arg_count);

/**
 * @brief Check if a string contains format placeholders
 * @param str String to check
 * @return true if string contains {} placeholders
 */
bool XvrFormatStringHasPlaceholders(const char* str);

#endif /* XVR_FORMAT_STRING_H */
