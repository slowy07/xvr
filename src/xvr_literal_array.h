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
 * @brief dynamic array of `Xvr_Literal` values
 *
 * `Xvr_literal array` providing growable, contigous array with
 *   - value semantics
 *   - automatic memory management (`Xvr_freeLiteralArray`)
 *   - O(1) amortized append, O(n) search / insert (standard vector behaviour)
 *
 * memory Layout:
 *     +----------------+----------------+----------------+
 *     | Xvr_Literal* literals | int capacity | int count |
 *     +----------------+----------------+----------------+
 *     `literals` points to heap-allocated array of `Xvr_Literal` (not
 * pointers!)
 *
 * design:
 * - storing value, not poinnters: avoiding indirection, improving cache
 * locality
 * - no realloc on shrink: capacity never decreases
 *
 */

#ifndef XVR_LITERAL_ARRAY_H
#define XVR_LITERAL_ARRAY_H

#include "xvr_common.h"
#include "xvr_literal.h"

/**
 * @struct Xvr_LiteralArray
 * @brief growable array of Xvr_Literal values
 *
 */
typedef struct Xvr_LiteralArray {
    Xvr_Literal* literals;  // heap-allocated buffer `Xvr_Literal`
    int capacity;           // number of allocated slots (>= count)
    int count;              // number of active elements (>= 0)
} Xvr_LiteralArray;

/**
 * @brief initializes an empty `Xvr_LiteralArray` (zero allocation)
 *
 * @param[out] array to initialize
 * set:
 * - literals = NULL
 *   - capacity = 0
 *   - count = 0
 */
XVR_API void Xvr_initLiteralArray(Xvr_LiteralArray* array);

/**
 * @brief free all resource held by the array
 *
 * @param[in, out] array array to desttoy (must not be NULL)
 */
XVR_API void Xvr_freeLiteralArray(Xvr_LiteralArray* array);

/**
 * @brief appends a literal to the end of the array
 *
 * @param[in, out] array array to modify (must not be NULL)
 * @param[in] literal literal to append (copied via `Xvr_copyLiteral`)
 * @return 0 on success, -1 on allocation failure (out of memory)
 *
 * behaviour:
 * - if `count == capacity` grows buffe (typically x1.5 or x2)
 *   - stores `deep copy` of `literal` at `literals[count]`
 *   - increment `count`
 */
XVR_API int Xvr_pushLiteralArray(Xvr_LiteralArray* array, Xvr_Literal literal);

/**
 * @brief removes and return the last element
 *
 * @param[in, out] array array to pop from (must not be NULL)
 * @return Popped literal (moved out - called owns), or `XVR_TO_NULL_LITERAL` if
 * empty
 *
 * behaviour:
 * - if `count == 0` return `null` leaves array unchanged
 *   - otherwise: decrement `count`, return `literals[count]` (without freeing
 * it)
 *   - callers is responsible for Xvr_freeLiteral() if not reusing
 *
 *   @note does not shrink `capacity` (amortized O(1)) no allocation / failure
 * possible
 */
XVR_API Xvr_Literal Xvr_popLiteralArray(Xvr_LiteralArray* array);

/**
 * @brief set element at `index` to `value`
 *
 * @param[in, out] array array to modify
 * @param[in] index index literal (must be `XVR_LITERAL_INTEGER` or
 * `XVR_LITERAL_INDEX_BLANK`)
 * @param[in] value new value (copied `Xvr_copyLiteral`)
 * @return `true` on success `false` if:
 *     - index is not integer / blank
 *     - index is negative (unless handling negatives indices)
 *     - OOM during resize / copy
 *
 * index semantics:
 *   - index >= count: grows array (fills holes with
 * `XVR_TO_INDEX_BLANK_LITERAL`)
 * - index < 0: undefined (future: Python-Style negative indexing)
 * - `XVR_LITERAL_INDEX_BLANK`: set hole at `index` (special case)
 */
XVR_API bool Xvr_setLiteralArray(Xvr_LiteralArray* array, Xvr_Literal index,
                                 Xvr_Literal value);

/**
 * @brief linear search for first occurrence of literal
 *
 * @param[in] array array to seach
 * @param[in] literal value to find (compared via `Xvr_literalsAreEqual`)
 * @return index (>= 0) if found, -1 if not found or error
 *
 * @note O(n) use only for small arrays or infrequent lookups
 */
XVR_API Xvr_Literal Xvr_getLiteralArray(Xvr_LiteralArray* array,
                                        Xvr_Literal index);

int Xvr_findLiteralIndex(Xvr_LiteralArray* array, Xvr_Literal literal);

#endif  // !XVR_LITERAL_ARRAY_H
