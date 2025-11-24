/**
MIT License

Copyright (c) 2025 arfy slowy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

/**
 * @brief token type enumeration for lexer and parser
 *
 * token categories:
 *   - types (0 - 9): runtime type descriptors
 *   - keywords (10 - 30): reserved words
 *   - literal (31 - 36): values and identifiers
 *   - math ops (37 - 47): arithmetic and assignment
 *   - logical ops (48 - 62): parentheses, comparison, logic
 *   - other ops (63 - 69): punctuation
 *   - meta (70 - 72): lexer / parser control
 *
 * threading:
 *   - thread-safe for read-only access (no mutable state)
 *   - token types are immutable constants
 */

#ifndef XVR_TOKEN_TYPES_H
#define XVR_TOKEN_TYPES_H

/**
 * @enum Xvr_TokenType
 * @brief enumeration of all possible lexical tokens
 *
 * @note value are explicit (no auto-increment) to preserve binary compatibilty
 *  add new tokens only at end to maintain serialization
 */
typedef enum Xvr_TokenType {
    // types
    XVR_TOKEN_NULL,
    XVR_TOKEN_BOOLEAN,
    XVR_TOKEN_INTEGER,
    XVR_TOKEN_FLOAT,
    XVR_TOKEN_STRING,
    XVR_TOKEN_ARRAY,
    XVR_TOKEN_DICTIONARY,
    XVR_TOKEN_FUNCTION,
    XVR_TOKEN_OPAQUE,
    XVR_TOKEN_ANY,

    // keywords and reserved words
    XVR_TOKEN_AS,
    XVR_TOKEN_ASSERT,
    XVR_TOKEN_BREAK,
    XVR_TOKEN_CLASS,
    XVR_TOKEN_CONST,
    XVR_TOKEN_CONTINUE,
    XVR_TOKEN_DO,
    XVR_TOKEN_ELSE,
    XVR_TOKEN_EXPORT,
    XVR_TOKEN_FOR,
    XVR_TOKEN_FOREACH,
    XVR_TOKEN_IF,
    XVR_TOKEN_IMPORT,
    XVR_TOKEN_IN,
    XVR_TOKEN_OF,
    XVR_TOKEN_PRINT,
    XVR_TOKEN_RETURN,
    XVR_TOKEN_TYPE,
    XVR_TOKEN_ASTYPE,
    XVR_TOKEN_TYPEOF,
    XVR_TOKEN_VAR,
    XVR_TOKEN_WHILE,

    // literal values
    XVR_TOKEN_IDENTIFIER,
    XVR_TOKEN_LITERAL_TRUE,
    XVR_TOKEN_LITERAL_FALSE,
    XVR_TOKEN_LITERAL_INTEGER,
    XVR_TOKEN_LITERAL_FLOAT,
    XVR_TOKEN_LITERAL_STRING,

    // math operators
    XVR_TOKEN_PLUS,
    XVR_TOKEN_MINUS,
    XVR_TOKEN_MULTIPLY,
    XVR_TOKEN_DIVIDE,
    XVR_TOKEN_MODULO,
    XVR_TOKEN_PLUS_ASSIGN,
    XVR_TOKEN_MINUS_ASSIGN,
    XVR_TOKEN_MULTIPLY_ASSIGN,
    XVR_TOKEN_DIVIDE_ASSIGN,
    XVR_TOKEN_MODULO_ASSIGN,
    XVR_TOKEN_PLUS_PLUS,
    XVR_TOKEN_MINUS_MINUS,
    XVR_TOKEN_ASSIGN,

    // logical operators
    XVR_TOKEN_PAREN_LEFT,
    XVR_TOKEN_PAREN_RIGHT,
    XVR_TOKEN_BRACKET_LEFT,
    XVR_TOKEN_BRACKET_RIGHT,
    XVR_TOKEN_BRACE_LEFT,
    XVR_TOKEN_BRACE_RIGHT,
    XVR_TOKEN_NOT,
    XVR_TOKEN_NOT_EQUAL,
    XVR_TOKEN_EQUAL,
    XVR_TOKEN_LESS,
    XVR_TOKEN_GREATER,
    XVR_TOKEN_LESS_EQUAL,
    XVR_TOKEN_GREATER_EQUAL,
    XVR_TOKEN_AND,
    XVR_TOKEN_OR,

    // other operators
    XVR_TOKEN_QUESTION,
    XVR_TOKEN_COLON,
    XVR_TOKEN_SEMICOLON,
    XVR_TOKEN_COMMA,
    XVR_TOKEN_DOT,
    XVR_TOKEN_PIPE,
    XVR_TOKEN_REST,

    // meta tokens
    XVR_TOKEN_PASS,
    XVR_TOKEN_ERROR,
    XVR_TOKEN_EOF,
} Xvr_TokenType;

#endif  // !XVR_TOKEN_TYPES_H
