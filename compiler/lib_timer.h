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
 * @brief timer-based hook for the runtime
 *
 * Xvr_hookTimer implement a system hook that provides time-related services
 *   - current time retrival: time.now() -> Xvr_Literal with timestamp
 *   - time formatting : time.format() -> string representation
 *   - sleep functionally: time.sleep(seconds) -> pause execution
 *   - performance timing: time.elapsed() -> high resultion duration
 *
 * memory management:
 *   - hook owns returned literals (transferred to interpreter scope)
 *   - no persistent allocation - stateless function
 *   - safe to call multiple times
 *
 * threading:
 *   - safe for read-only access (no mutable state)
 *   - system time calls are inherently thread-safe
 */

#ifndef LIB_TIMER_H
#define LIB_TIMER_H

#include "../src/xvr_interpreter.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * @brief system hook for time-related functionally
 *
 * @param[in, out] interpreter interpreter context (for scope access, error
 * reporting)
 * @param[in] identifier requested identifier
 * @param[in] alias optional alias
 * @return 0 on success, negative value on error
 *   - -1: invalid identifier
 *   - -2: system time unavailable
 *   - -3: OOM during literala creation
 *   - -4: interpeter in panic mode
 *
 * @note hook is stateless - no persistent data stored, time function are boudn
 * to current scope via interpreter
 */
int Xvr_hookTimer(Xvr_Interpreter* interpreter, Xvr_Literal identifier,
                  Xvr_Literal alias);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // !LIB_TIMER_H
