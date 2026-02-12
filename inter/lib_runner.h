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
 * @brief filesystem driver and path resolution system for runtime
 *
 * Xvr_libRunner providing virtual filesystem layer
 * - driver registration: maps virtual paths to pyhsical locations
 *   - path resolution: converts virtual paths to real filesystem path
 *   - driver mapping: support multiple mount pointes
 *   - security: validates paths to preventing directory traversal attacks
 *   - interpreter hooks: providing fs module wiht file operations
 *
 * memory management:
 *   - global drive dictionary is owned by library (init / free required)
 *   - file handles are `XVR_LITERAL_OPAQUE` with `XVR_OPAQUE_TAG_RUNNER` tag
 *   - path literal are managed by interpreter literal system
 */

#ifndef LIB_RUNNER_H
#define LIB_RUNNER_H

#include "xvr_interpreter.h"
#include "xvr_common.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * @def XVR_OPAQUE_TAG_RUNNER
 * @brief unique tag for filesystem-related opaque objects
 *
 * @note value is explicitly chosen to avoid conflicts with other system tags,
 * should be unique across entire runtime
 */
#define XVR_OPAQUE_TAG_RUNNER 100

/**
 * @brief system hook for filesystem functionally
 *
 * @param[in, out] interpreter interpreter context
 * @param[in] identifier requested identifier
 * @param[in] alias optional alias
 *
 * @return 0 on success, negative value on error:
 *   - -1: invalid identifier
 *   - -2: system resource unavailable
 *   - -3: OOM during module creation
 *   - -4: interpreter in panic mode
 *
 * @note require `Xvr_initDriveDictionary()` to be called first, all file
 * operations are validated againts registered drives
 */
int Xvr_hookRunner(Xvr_Interpreter* interpreter, Xvr_Literal identifier,
                   Xvr_Literal alias);

/**
 * @brief initialize global drive dictionary
 *
 * @note must be called before any other runner functions, safe to call multiple
 * which is idempotent
 * @arning not thread-safe -> call during single-threaded initialization
 */
XVR_API void Xvr_initDriveDictionary(void);

/**
 * @brief free global drive dictionary and registered drives
 *
 * @note safe to call even if `Xvr_initDriveDictionary()` was never called, safe
 * to call multiple times which is idempotent
 * @warning all file handle created by runner system becomes invalid after this
 * call
 * @warning not thread-safe - ensure no concurrent access before calling
 */
XVR_API void Xvr_freeDriveDictionary(void);

/**
 * @brief gets poitner to global drive dictionary
 *
 * @return pointer to drive dictionary, or NULL if not initialize
 *
 * dictionary mapping:
 *  - key: virtual path prefix
 *  - value: pyhsical path
 *
 * @note returned pointer is valid until `Xvr_freeDriveDictionary()` is called,
 * dictionary is shared across all interpreters - use external synchronization
 */
XVR_API Xvr_LiteralDictionary* Xvr_getDriveDictionary(void);

/**
 * @brief resolve virtual path literal to pyhsical filesystem path
 *
 * @param[in] interpreter interpreter context
 * @param[in, out] driverPathLiteral virtual path literal to resolve (modified
 * in-place)
 * @return pyhsical as `XVR_LITERAL_STRING` or `XVR_TO_NULL_LITERAL` on error:
 *   - invalid path literal type
 *   - path not found in drive dictionary
 *   - path traversal detected
 *   - security validation failed
 *
 * @note input literal may be modified - called should re-check type after call.
 * return literal is owned by caller - call `Xvr_freeLiteral` when done
 * @warning path validation is critical - never using unvalidated paths in
 * system calls
 */
Xvr_Literal Xvr_getFilePathLiteral(Xvr_Interpreter* interpreter,
                                   Xvr_Literal* driverPathLiteral);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // !LIB_RUNNER_H
