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
 * @brief unified memory management interface for XVR runtime
 *
 * providing:
 *  - a single, flexible reallocation
 *  - macro wrappers for common operations
 *  - pluggable allocator backend (for arenas, GC, debug allocators)
 *
 * principle of design:
 *  - zero external dependencies
 *  - explicit size tracking
 *  - growth policy separation
 *  - type-safe macros
 *  - deterministic behaviour
 *
 * memory Contract for `Xvr_reallocate`:
 *   | pointer | oldSize | newSize | Behavior                              |
 *   |---------|---------|---------|---------------------------------------|
 *   | NULL    | 0       | >0      | Allocate `newSize` bytes              |
 *   | p       | >0      | >0      | Resize `p` ->`newSize` (reserve min(old,
 * new))
 *   | |        p       | >0      | 0       | Free `p` | | NULL    | 0       | 0
 * | No-op (return NULL)                   |
 *
 * @note all macros are `nul-safe` and `size-safe` - missues yields compile-time
 * errors
 */

#ifndef XVR_MEMORY_H
#define XVR_MEMORY_H

#include "xvr_common.h"

#define XVR_ALLOCATE(type, count) \
    ((type*)Xvr_reallocate(NULL, 0, sizeof(type) * (count)))
#define XVR_FREE(type, pointer) Xvr_reallocate(pointer, sizeof(type), 0)
#define XVR_GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define XVR_GROW_CAPACITY_FAST(capacity) ((capacity) < 32 ? 32 : (capacity) * 2)
#define XVR_GROW_ARRAY(type, pointer, oldCount, count)               \
    (type*)Xvr_reallocate((type*)pointer, sizeof(type) * (oldCount), \
                          sizeof(type) * (count))
#define XVR_SHRINK_ARRAY(type, pointer, oldCount, count)             \
    (type*)Xvr_reallocate((type*)pointer, sizeof(type) * (oldCount), \
                          sizeof(type) * (count))
#define XVR_FREE_ARRAY(type, pointer, oldCount) \
    Xvr_reallocate((type*)pointer, sizeof(type) * (oldCount), 0)

void* Xvr_reallocate(void* pointer, size_t oldSize, size_t newSize);

/**
 * @typedef Xvr_MemoryAllocatorFn
 * @brief custom memory allocation callback
 *
 * @param[in, out] pointer existing allocation (NULL for first alloc)
 * @param[in] oldSize previous size in bytes
 * @param[in] newSize desired size in bytes (0 to free)
 * @return
 *  - on alloc / resize: pointer to newSize - byte buffer
 *  - on free
 *
 * requirements:
 *   - preserve min(oldSize, newSize) bytes on resize
 *   - return NULL on OOM
 *   - be thread safe if used in multi-threaded context
 */
typedef void* (*Xvr_MemoryAllocatorFn)(void* pointer, size_t oldSize,
                                       size_t newSize);

/**
 * @brief core memory reallocation primitive
 *
 * @param[in, out] pointer current allocation (NULL allowed)
 * @param[in] oldSize current size in bytes
 * @param[in] newSize desired size in bytes (0 to free)
 * @return pointer to reallocation memory, or NULL on failure / free
 *
 * @note this is the only memory function the runtime should call directly all
 * higher-level allocators.
 */
XVR_API void Xvr_setMemoryAllocator(Xvr_MemoryAllocatorFn);

#endif  // !XVR_MEMORY_H
