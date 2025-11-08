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

#ifndef XVR_LITERAL_DICTIONARY_H
#define XVR_LITERAL_DICTIONARY_H

#include "xvr_common.h"
#include "xvr_literal.h"

#define XVR_DICTIONARY_MAX_LOAD 0.75

typedef struct Xvr_private_entry {
    Xvr_Literal key;
    Xvr_Literal value;
} Xvr_private_entry;

typedef struct Xvr_LiteralDictionary {
    Xvr_private_entry* entries;
    int capacity;
    int count;
    int contains;
} Xvr_LiteralDictionary;

XVR_API void Xvr_initLiteralDictionary(Xvr_LiteralDictionary* dictionary);
XVR_API void Xvr_freeLiteralDictionary(Xvr_LiteralDictionary* dictionary);

XVR_API void Xvr_setLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                                      Xvr_Literal key, Xvr_Literal value);
XVR_API Xvr_Literal Xvr_getLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                                             Xvr_Literal key);
XVR_API void Xvr_removeLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                                         Xvr_Literal key);

XVR_API bool Xvr_existsLiteralDictionary(Xvr_LiteralDictionary* dictionary,
                                         Xvr_Literal key);

#endif  // !XVR_LITERAL_DICTIONARY_H
