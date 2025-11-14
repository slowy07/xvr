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
 * @brief compound data structure library for the runtime
 *
 * Xvr_LibCompound proveds a comprehensive compound data structure system
 *
 * memory management:
 *   - all operations respect  Xvr_Literal ownership semantics
 *   - deep copying for `clone` operations
 *   - automatic reference counting for nested structures
 *
 * threading:
 *   - no atomic operations on shared compound data
 *   - compound operations assume single-threaded access
 */

#ifndef LIB_COMPOUND_H
#define LIB_COMPOUND_H

#include "xvr_interpreter.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * @brief system hook for compound data structure functionally
 *
 * @param[in, out] interpreter interpreter
 * @param[in] identifier requested identifier
 * @param[in] alias optional alias
 * @return 0 if success, negative value on error:
 *   - -1: invalid identifier
 *   - -2: system resources unavailable
 *   - -3: OOM during module creation
 *   - -4: interpreter in panic mode
 *
 * @note all function perform runtime type check, operations that modify data
 * structure are deep-copy safe
 */
int Xvr_hookCompound(Xvr_Interpreter* interpreter, Xvr_Literal identifier,
                     Xvr_Literal alias);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // !LIB_COMPOUND_H
