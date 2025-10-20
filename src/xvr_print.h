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

#ifndef XVR_PRINT_H
#define XVR_PRINT_H

#include "xvr_common.h"

// TODO: adding parameter to specific error
typedef void (*Xvr_callbackType)(const char *);

XVR_API void Xvr_print(const char *msg);
XVR_API void Xvr_error(const char *msg);
XVR_API void Xvr_assertFailure(const char *msg);

XVR_API void Xvr_setPrintCallback(Xvr_callbackType cb);
XVR_API void Xvr_setErrorCallback(Xvr_callbackType cb);
XVR_API void Xvr_setAssertFailureCallback(Xvr_callbackType cb);

XVR_API void Xvr_resetPrintCallback(void);
XVR_API void Xvr_resetErrorCallback(void);
XVR_API void Xvr_resetAssertFailureCallback(void);

#endif // !XVR_PRINT_H
