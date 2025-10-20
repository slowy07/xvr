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

#ifndef XVR_COMMON_H
#define XVR_COMMON_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__linux__)
#    define XVR_API extern
#elif defined(_WIN32) || defined(_WIN64)
#    if defined(XVR_EXPORT)
#        define XVR_API __declspec(dllexport)
#    elif defined(XVR_IMPORT)
#        define XVR_API __declspec(dllimport)
#    else
#        define XVR_API extern
#    endif
#elif defined(__APPLE__)
#    define XVR_API extern
#else
// generic solution
#    define XVR_API extern
#endif

// XVR_BITNESS is used to encourage memory-cache friendliness
#if defined(__linux__)
#    if defined(__LP64__)
#        define XVR_BITNESS 64
#    else
#        define XVR_BITNESS 32
#    endif
#elif defined(_WIN32) || defined(_WIN64)
#    if defined(_WIN64)
#        define XVR_BITNESS 64
#    else
#        define XVR_BITNESS 32
#    endif
#elif defined(__APPLE__)
#    if defined(__LP64__)
#        define XVR_BITNESS 64
#    else
#        define XVR_BITNESS 32
#    endif
#else
// generic solution
#    define XVR_BITNESS -1
#endif

// bytecode version specifiers, embedded as the header
#define XVR_VERSION_MAJOR 0
#define XVR_VERSION_MINOR 1
#define XVR_VERSION_PATCH 0

// defined as a function, for technical reasons
#define XVR_VERSION_BUILD Xvr_private_version_build()
XVR_API const char* Xvr_private_version_build(void);

#endif  // !XVR_COMMON_H
