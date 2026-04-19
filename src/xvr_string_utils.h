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

#ifndef XVR_STRING_UTILS_H
#define XVR_STRING_UTILS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t xvr_safe_strlen(const char* str, size_t max_len);

size_t xvr_safe_strlen_bounded(const char* str, size_t max_len);

int xvr_safe_strcmp(const char* a, const char* b);

int xvr_safe_strncmp(const char* a, const char* b, size_t n);

char* xvr_safe_strdup(const char* str, size_t max_len);

#ifdef __cplusplus
}
#endif

#endif /* XVR_STRING_UTILS_H */
