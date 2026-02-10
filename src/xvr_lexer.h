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
 * @brief lexical analyzer (tokenizer)
 *
 * Xvr_Lexer implementing character-by-character, single-pass tokenizer
 * - converting source code string into Xvr_Token stream
 * - recognizes keywords, identifier, literals, operator and punctuation
 * - handle string / number escaping and validation
 * - maintain line / column tracking for error reporting
 *
 * memory management
 *   - lexer borrows source string
 *   - tokens reference substring of `source` (no allocation)
 *   - no temporary buffers
 *
 * threading:
 *   - no thread-safe -> external synchronization required
 *   - no atomic operations on lexer state
 *
 * using for:
 *   - fast tokenization O(n)
 *   - small memory footprint (no intermediate buffers)
 *   - error recovery (continues after invalid tokens)
 *   - debugging
 */

#ifndef XVR_LEXER_H
#define XVR_LEXER_H

#include "xvr_common.h"
#include "xvr_token_types.h"

/**
 * @struct Xvr_Lexer
 * @brief lexer state machine - source code input to token stream
 *
 * @note size: ~24 bytes - designing for stack allocation if needed
 */
typedef struct {
    const char* source;  // input source code
    int start;           // start offset of current token being built
    int current;         // current character position in source
    int line;            // current line number
} Xvr_Lexer;

/**
 * @struct Xvr_Token
 * @brief single lexical token
 *
 * @note `lexeme` is not null-terminated - use `length` for substring access
 */
typedef struct {
    Xvr_TokenType type;  // token classification
    const char* lexeme;  // pointer to original text in source
    int length;          // length of token text in btyes
    int line;            // line number where token starts
} Xvr_Token;

/**
 * @brief intializes lexer with source code
 *
 * @param[out] lexer lexer to initialize (must be not NULL)
 * @param[in] source srouce code string to tokenize
 *
 * @note safe to call on zeroed memory, calling twice is safe which is
 * idempotent
 * @warning `source` must outlive lexer - lexer borrows pointer
 * @warning `source` must be null-terminated
 */
XVR_API void Xvr_initLexer(Xvr_Lexer* lexer, const char* source);

/**
 * @brief scan next token from source code
 *
 * @param[in, out] lexer lexer state
 * @return next token in stream
 *
 * - advance lexer state to consume token next
 *   - return `XVR_TOKEN_EOF` when source is exhausted
 *   - return `XVR_TOKEN_ERROR` on invalid input
 *
 * @note returned token `lexeme` points into original `source` string, token is
 * valid only as long as `source` exists.
 */
XVR_API Xvr_Token Xvr_private_scanLexer(Xvr_Lexer* lexer);

/**
 * @brief print token details to stdout for debugging
 *
 * @param[in] token token to print
 *
 * @note itended for debugging and testing, does not print to interpreter output
 * function.
 */
XVR_API void Xvr_private_printToken(Xvr_Token* token);

#endif  // !XVR_LEXER_H
