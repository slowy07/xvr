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
 * @brief reference-counted , immutable string type for efficient sharing and
 * memory reuse
 *
 * Xvr_RefString implement copy on write, heap allocated string with
 *   - embedded length
 *   - manual reference couting (no GC overhead)
 *   - flexible allocation control (custom allocator hooks)
 *   - immutable content post-creation (thread-safe reads)
 *
 *   memory layout
 *     +----------------+----------------+----------------+
 *     | size_t length  | int refCount   | char data[length+1] (NUL-terminated)
 *     +----------------+----------------+----------------+
 *     ^                ^                ^
 *     Xvr_RefString*   |                |
 *                      +-- points here  +-- data[0]
 *
 * the struct is allocated as a single contiguous block:
 *  malloc(sizeof(Xvr_RefString) + length + 1)
 *
 * @warn type is not thread-safe for concurrent modification
 * @note this will inspiring from LLVM `StringRef` / `Twine` and GObjects, but
 * with explicit refcount control
 */

#ifndef XVR_REFSTRING_H
#define XVR_REFSTRING_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @typedef Xvr_RefStringAllocatorFn
 * @brief custom memory reallocation callback for `Xvr_RefString` allocation
 *
 * @param [in out] pointer existing allocation (NULL on first alloc)
 * @param [in] oldSize previous allocated size in bytes (0 if pointer == NULL)
 * @param [in] newSize desire new size in bytes
 * @return new pointer on success
 *
 * @note default: using `realloc() / free()` via internal fallback, call
 * Xvr_setRefStringAllocatorFn(NULL) to reset default
 */
typedef void* (*Xvr_RefStringAllocatorFn)(void* pointer, size_t oldSize,
                                          size_t newSize);

/**
 * @bfief sets the global allocator function for all `Xvr_RefString` operations
 * @param[in]  fn allocator function (NULL reset to built-in `realloc / free`
 */
void Xvr_setRefStringAllocatorFn(Xvr_RefStringAllocatorFn);

/**
 * @struct reference-counted, immutable string with embedded NULL terminator
 *
 * - length: number of bytes in data (excluding final NULL)
 * - refCount: current number of live strong reference ( >= 1 after creation)
 * - `data[]`: flexible array of length + 1 bytes: [0..length - 1] = content,
 * [length] = '\0'
 */
typedef struct Xvr_RefString {
    size_t length;
    int refCount;
    char data[];
} Xvr_RefString;

/**
 * @brief create a new `Xvr_RefString` from NULL-terminated C string
 *
 * @param [in] cstring source string (must not be NULL) behaviour undefined if
 * NULL
 * @return new `Xvr_RefString` with `refCount = 1`, or NULL on allocation
 * failure
 */
Xvr_RefString* Xvr_createRefString(const char* cstring);

/**
 * @brief create new `Xvr_RefString` from a buffer o known length
 *
 * @param[in] cstring source buffer (may contains NULL bytes' must not be NULL
 * if `length` > 0)
 * @param[in] length number of bytes to copy (>= 0)
 * @return new `Xvr_RefString` with `refCount = 1`, or NULL on allocation
 * failure
 *
 * - copies exactly `length` bytes (even if cstring contains early NULs)
 */
Xvr_RefString* Xvr_createRefStringLength(const char* cstring, size_t length);

/**
 * @brief decrement reference count and frees memory if count reaches zero
 *
 * @param[in, out] refString string to release (must not be NULL) No-op if NULL
 */
void Xvr_deleteRefString(Xvr_RefString* refString);

/**
 * @brief returns current reference count
 *
 * @param[in] refString string to inspect
 * @return reference count (>= 0) return 0 only if object is being destroyed
 */
int Xvr_countRefString(Xvr_RefString* refString);

/**
 * @brief returns string length in bytes (excluding final NULL)
 *
 * @param[in] refString string to inspect (must not be NULL)
 * @return `length` field ((O)1, no scan)
 */
size_t Xvr_lengthRefString(Xvr_RefString* refString);

/**
 * @brief increments reference count and return same pointer (shallow copy)
 *
 * @param[in, out] refString string to share (must not be NULL)
 * @return `refString` (same pointer), or NULL  if `refSTring == NULL`
 *
 * @note to transfer ownership without copy , just assigning the pointer
 */
Xvr_RefString* Xvr_copyRefString(Xvr_RefString* refString);

/**
 * @brief create a new independent copy (deep copy) of the string
 *
 * @param[in] refString string to duplicate (must not be NULL)
 * @return new `Xvr_RefString` with `refCount =1`, or NULL on allocation failure
 */
Xvr_RefString* Xvr_deepCopyRefString(Xvr_RefString* refString);

/**
 * @brief return immutable C-string view (const char*) of the data
 *
 * @param[in] refString string to view (must not be NULL)
 * @return pointer to `refString->data` (NULL-terminated, valid for object
 * lifetime)
 *
 * - O(1), zero copy
 */
const char* Xvr_toCString(Xvr_RefString* refString);

/**
 * @brief compares two `Xvr_RefString` for byte-wise equality
 *
 * @param[in] lhs left operand
 * @param[in] rhs right operand
 * @return true iff both are NULL, or noth non-NULL
 */
bool Xvr_equalsRefString(Xvr_RefString* lhs, Xvr_RefString* rhs);

/**
 * @brief compares `Xvr_RefString` with a raw C string
 *
 * @param[in] lhs left operand
 * @param[in] cstring right operand
 * @return `true` iff `lhs->length == strlen(cstring)` and `memcmp` matches
 */
bool Xvr_equalsRefStringCString(Xvr_RefString* lhs, char* cstring);

#endif  // !XVR_REFSTRING_H
