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
 * @brief recursive descent parser for the XVR
 *
 * Xvr_Parser implemeting pratt-style recursive descent parser with
 *   - single token lookahead
 *   - panic-mode error recovery
 *   - operator precedence climbing for expression
 *   - automatic AST node generation
 *   - state tracking for multi-pass compilation
 *
 * memory management:
 *   - parser owns its Xvr_Lexer
 *   - generated AST nodes are owned by caller
 *   - no temporary allocations
 */

#ifndef XVR_PARSER_H
#define XVR_PARSER_H

#include "xvr_ast_node.h"
#include "xvr_common.h"
#include "xvr_lexer.h"

/**
 * @struct Xvr_Parser
 * @brief parser state machine - token lookahead, error flags, and lexer
 * reference
 *
 * fields:
 *  - lexer: source of tokens
 *  - error: set to true if any syntax error occured during parsing
 *  - panic: set to true if currently in error recovery mode
 *  - current: current token to process
 * - previous: last consumed token
 *
 *   @note size are: 48 bytes (2 tokens + 2 ptrs + 2 bools) -> fits in registers
 * / cache line
 */
typedef struct {
    Xvr_Lexer* lexer;  // source of tokens
    bool error;        // true if any syntax error occured
    bool panic;        // true if in error recovery mode

    Xvr_Token current;   // current token to process
    Xvr_Token previous;  // last consumed token
} Xvr_Parser;

/**
 * @brief initializes parser with a lexer
 *
 * @param[out] parser parser to initializes (must not be NULL)
 * @param[in] lexer token source
 *
 * sets:
 *   - lexer = lexer
 * - error = false, panic = false
 * - current = next token from lexer (or EOF if empty)
 *
 * @note this safe to call on zeroed memory: calling twice are safe but
 * overwrites lexer
 */
XVR_API void Xvr_initParser(Xvr_Parser* parser, Xvr_Lexer* lexer);

/**
 * @brief frees parser resource / does not free lexer
 *
 * @param[in, out] parser parser to destroy
 *
 * @note safe to call even if parser->error == true
 */
XVR_API void Xvr_freeParser(Xvr_Parser* parser);

/**
 * @brief parsers the entire input stream into AST
 *
 * @param[in, out] parser parser state
 * @return root AST node of parsed program, or NULL on
 *   - syntax error
 *   - empty input
 *   - out of memory during AST construction
 *
 * error recovery:
 *  - if syntax are error: setting `parser->error = true`, enters panic mode
 *  - in panic mode: skips tokens until `;`, '}' or newline
 *  - may return partial AST even if error == true
 */
XVR_API Xvr_ASTNode* Xvr_scanParser(Xvr_Parser* parser);

#endif  // !XVR_PARSER_H
