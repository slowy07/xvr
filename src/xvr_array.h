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

#ifndef XVR_ARRAY_H
#define XVR_ARRAY_H
#include "xvr_common.h"
#include "xvr_value.h"

typedef struct Xvr_Array { // 32 | 64 BITNESS
  unsigned int capacity;   // 4  | 4
  unsigned int count;      // 4  | 4
  Xvr_Value data[];        //-  | -
} Xvr_Array;               // 8  | 8

XVR_API Xvr_Array *Xvr_resizeArray(Xvr_Array *array, unsigned int capacity);

#ifndef XVR_ARRAY_INITIAL_CAPACITY
#define XVR_ARRAY_INITIAL_CAPACITY 8
#endif // !XVR_ARRAY_INITIAL_CAPACITY

#ifndef XVR_ARRAY_EXPANSION_RATE
#define XVR_ARRAY_EXPANSION_RATE 2
#endif // !XVR_ARRAY_EXPANSION_RATE

#ifndef XVR_ARRAY_ALLOCATE
#define XVR_ARRAY_ALLOCATE() Xvr_resizeArray(NULL, XVR_ARRAY_INITIAL_CAPACITY)
#endif // !XVR_ARRAY_ALLOCATE

#ifndef XVR_ARRAY_FREE
#define XVR_ARRAY_FREE(array) (array = Xvr_resizeArray(array, 0))
#endif // !XVR_ARRAY_FREE

#ifndef XVR_ARRAY_EXPAND
#define XVR_ARRAY_EXPAND(array)                                                \
  (array = (array != NULL && (array)->count + 1 > (array)->capacity            \
                ? Xvr_resizeArray(array, (array)->capacity *                   \
                                             XVR_ARRAY_EXPANSION_RATE)         \
                : array))
#endif // !XVR_ARRAY_EXPAND

#ifndef XVR_ARRAY_PUSHBACK
#define XVR_ARRAY_PUSHBACK(array, value) (XVR_ARRAY_EXPAND(array), (array)->data[(array)->count++] = (value))
#endif // !XVR_ARRAY_PUSHBACK

#endif // !XVR_ARRAY_H
