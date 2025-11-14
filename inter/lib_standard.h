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
 * @brief standard library hook for the runtime
 *
 * Xvr_hookStandard implements a system hook that provides standard library
 *   - global constant: null, true, false
 *   - builtin function: print, assert
 *
 * memory management:
 *   - hook registration: Xvr_injectNativeHook(interpreter, "std",
 * Xvr_hookStandard)
 *   - interpeter access: called during interpreter resolution
 *   - literal system: return XVR_LITERAL_FUNCTION_NATIVE object
 *
 * @warning hook execution may be fail if system resource unavailable. always
 * check return codes in critical paths
 */

#ifndef LIB_STANDARD_H
#define LIB_STANDARD_H

#include "../src/xvr_interpreter.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * @brief system hook for standard library functionality
 *
 * @param[in, out] interpreter interpreter context
 * @param[in] identifier requrested identifier
 * @param[in] alias optional alias
 * @return 0 on success, negative value on error:
 *   - -1: invalid system
 *   - -2: system resource unavailable
 *   - -3: OOM during module creation
 *   - -4: interpeter in panic mode
 *
 * @note hook is stateless - no persistent data stored in hook itself, module
 * objects are stored in interpreter scope
 */
int Xvr_hookStandard(Xvr_Interpreter* interpreter, Xvr_Literal identifier,
                     Xvr_Literal alias);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // !LIB_STANDARD_H
