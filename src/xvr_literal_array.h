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

#ifndef XVR_LITERAL_ARRAY_H
#define XVR_LITERAL_ARRAY_H

#include "xvr_common.h"
#include "xvr_literal.h"

typedef struct Xvr_LiteralArray {
    Xvr_Literal* literals;
    int capacity;
    int count;
} Xvr_LiteralArray;

XVR_API void Xvr_initLiteralArray(Xvr_LiteralArray* array);
XVR_API void Xvr_freeLiteralArray(Xvr_LiteralArray* array);
XVR_API int Xvr_pushLiteralArray(Xvr_LiteralArray* array, Xvr_Literal literal);
XVR_API Xvr_Literal Xvr_popLiteralArray(Xvr_LiteralArray* array);
XVR_API bool Xvr_setLiteralArray(Xvr_LiteralArray* array, Xvr_Literal index,
                                 Xvr_Literal value);
XVR_API Xvr_Literal Xvr_getLiteralArray(Xvr_LiteralArray* array,
                                        Xvr_Literal index);

int Xvr_findLiteralIndex(Xvr_LiteralArray* array, Xvr_Literal literal);

#endif  // !XVR_LITERAL_ARRAY_H
