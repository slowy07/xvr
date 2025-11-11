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

#ifndef INTER_TOOLS_H
#define INTER_TOOLS_H

#include "xvr_common.h"

const char* Xvr_readFile(const char* path, size_t* fileSize);
int Xvr_writeFile(const char *path, const unsigned char *bytes, size_t size);
const unsigned char* Xvr_compileString(const char* source, size_t* size);

void Xvr_runBinary(const unsigned char* tb, size_t size);
void Xvr_runBinaryFile(const char* fname);
void Xvr_runSource(const char* source);
void Xvr_runSourceFile(const char* fname);

#endif // !INTER_TOOLS_H
