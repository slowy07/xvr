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
 * @brief bidirectional mapping between keyword strings and token types
 *
 * `Xvr_KeywordType` providing lookup table for converting between
 *   - keyword strings: if, var, return, etc
 *   - token type: XVR_TOKEN_IF, XVR_TOKEN_VAR, XVR_TOKEN_RETURN, etc.
 *
 * design:
 *   - fast lookup: O(1) array access for type -> keyword
 *   - fast search: O(n) linear search for keyword -> type (n = ~20 keyword)
 *   - memory safety: immutable lookup table
 *   - maintability: single source of truth for keywrod definitions
 *
 * memory management:
 *  - keyword string are static literals (no allocation)
 *  - array is read-only after initialization
 *  - no ownership transfer - all data is immutable
 */

#ifndef XVR_KEYWORD_TYPES_H
#define XVR_KEYWORD_TYPES_H

#include "xvr_token_types.h"

/**
 * @struct Xvr_KeywordType
 * @brief single entry in keyword-to-type lookup table
 *
 * field:
 *   - type: corresponding token type
 *   - keyword: null-terminated keyword string
 *
 * @note size: ~16 btyes on 64-bit system - designed for cache efficiency
 */
typedef struct {
    Xvr_TokenType type;  // token type corresponding to keyword
    char* keyword;       // null-terminated keyword string (static)
} Xvr_KeywordType;

/**
 * @var Xvr_KeywordType
 * @brief global keyword lookup table
 *
 * array format:
 *   - [0..N - 1]: valid keyword-type pairs
 * - [N]: sentinel `{XVR_TOKEN_EOF, NULL}` to mark end
 */
extern Xvr_KeywordType Xvr_keywordTypes[];

/**
 * @brief finds keyword string for given token type
 *
 * @param[in] type token type to lookup
 * @return keyword string if found (example: if), or NULL, if:
 *   - type is not keyword
 *   - type is out of range
 *   - internal table is corrupted
 *
 * @note performance: O(1) -> direct array access using `type` as index
 */
char* Xvr_findKeywordByType(Xvr_TokenType type);

/**
 * @brief find token type for given keyword string
 *
 * @param[in] keyword keyword string to lookup up (example if)
 * @return token type if found (example XVR_TOKEN_IF) or XVR_TOKEN_IDENTIFIER if
 * keywrod is not reserved word, or XVR_TOKEN_EOF on internal error
 *
 * @note performance: O(n) -> linear search through keyword table (n ~ 20)
 */
Xvr_TokenType Xvr_findTypeByKeyword(const char* keyword);

#endif  // !XVR_KEYWORD_TYPES_H
