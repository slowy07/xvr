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
 * @brief high-level tools for runtime
 *
 * Xvr_InterTools providing convenience function for common xvr operations
 * - execution
 * - file-based execution
 * - compilation
 *
 * memory management:
 *   - Xvr_readFile: returns malloc buffer (caller must free)
 *   - Xvr_compileString: returns malloc bytecode
 *   - all other functions manage their own memory internally
 *
 * threading:
 *   - not thread-safe: functions assume single-threaded execution
 *   - no atomic operations on shared state
 *   - external synchronization required for concurrent use
 *
 * @warning check retunrs values - file operations and compilation may fail.
 * memory returned `Xvr_readFile` and `Xvr_compileString` must be freed by
 * caller file paths subjects to OS security restrictions.
 */

#ifndef INTER_TOOLS_H
#define INTER_TOOLS_H

#include "xvr_common.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

/**
 * @brief reads entire file into memory buffer
 *
 * @param[in] path file path to read
 * @param[out] fileSize pointer to store file size
 * @return pointer to malloc buffer containing file contents or NULL on error:
 *   - file not found
 *   - permission denied
 *   - out of memory
 *   - I/O error during read
 */
const unsigned char* Xvr_readFile(const char* path, size_t* fileSize);

/**
 * @brief writes buffer to file
 *
 * @param[in] path file path to write
 * @param[in] bytes buffer to write
 * @param[in] size number of bytes to write
 * @return 0 on success, negative error code in failure:
 *   - -1: invalid arguments
 *   - -2: permission denied
 *   - -3: out of disk space
 *   - -4: I/O error during write
 *
 * @note create file if doesn't exist, overwrite if it does, parent directories
 * must exists
 */
int Xvr_writeFile(const char* path, const unsigned char* bytes, size_t size);

/**
 * @brief compile source code string to bytecode
 *
 * @param[in] source source code string to compile
 * @param[out] size pointer to store bytecode size
 * @return poitner to malloc bytecode buffer, NULL on error:
 *   - compilation error
 *   - invalid source
 *   - out of memory during bytecode generation
 */
const unsigned char* Xvr_compileString(const char* source, size_t* size);

/**
 * @brief execute bytecode directly from memory buffer
 *
 * @param[in] tb bytecode buffer to execute
 * @param[in] size size of bytecode buffer in bytes
 *
 * @note create temporary interpreter instance for execution, using default
 * output functions (`fprintf`, `fprintf(stderr, ...)`)
 */
void Xvr_runBinary(const unsigned char* tb, size_t size);

/**
 * @brief execute bytecode from file
 *
 * @param[in] fname file path containing bytecode
 *
 * @note read file into memory, then caling `Xvr_runBinary`, uses default output
 * function
 */
void Xvr_runBinaryFile(const char* fname);

/**
 * @brief execute source code string directly
 *
 * @param[in] source source code string to execute
 * @note compile source to bytecode in memory then execute
 */
void Xvr_runSource(const char* source);

/**
 * @brief execute source code from file
 *
 * @param[in] fname file path containing source code
 *
 * @note read files, compile source, then bytecode
 * @warning file must contain valid XVR source code
 */
void Xvr_runSourceFile(const char* fname);

/**
 * @brief read a line of input with line editing support
 *
 * @param[out] buffer buffer to store the input line
 * @param[in] size maximum size of buffer
 * @return pointer to buffer on success, NULL on EOF
 *
 * @note handles arrow keys, backspace, etc. for better REPL experience
 */
char* Xvr_readLine(char* buffer, int size);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // !INTER_TOOLS_H
