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

XVR_API void Xvr_resetPrintCallback();
XVR_API void Xvr_resetErrorCallback();
XVR_API void Xvr_resetAssertFailureCallback();

#endif // !XVR_PRINT_H
