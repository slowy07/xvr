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

#ifndef XVR_REFSTRING_H
#define XVR_REFSTRING_H

#include <stdbool.h>
#include <stddef.h>

typedef void* (*Xvr_RefStringAllocatorFn)(void* pointer, size_t oldSize,
                                          size_t newSize);
void Xvr_setRefStringAllocatorFn(Xvr_RefStringAllocatorFn);

typedef struct Xvr_RefString {
    size_t length;
    int refCount;
    char data[];
} Xvr_RefString;

Xvr_RefString* Xvr_createRefString(const char* cstring);
Xvr_RefString* Xvr_createRefStringLength(const char* cstring, size_t length);
void Xvr_deleteRefString(Xvr_RefString* refString);
int Xvr_countRefString(Xvr_RefString* refString);
size_t Xvr_lengthRefString(Xvr_RefString* refString);
Xvr_RefString* Xvr_copyRefString(Xvr_RefString* refString);
Xvr_RefString* Xvr_deepCopyRefString(Xvr_RefString* refString);
const char* Xvr_toCString(Xvr_RefString* refString);
bool Xvr_equalsRefString(Xvr_RefString* lhs, Xvr_RefString* rhs);
bool Xvr_equalsRefStringCString(Xvr_RefString* lhs, char* cstring);

#endif  // !XVR_REFSTRING_H
