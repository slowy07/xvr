#ifndef XVR_SCOPE_H
#define XVR_SCOPE_H

#include "xvr_bucket.h"
#include "xvr_common.h"
#include "xvr_string.h"
#include "xvr_table.h"
#include "xvr_value.h"

typedef struct Xvr_Scope {
  struct Xvr_Scope *next;
  Xvr_Table *table;
  unsigned int refCount;
} Xvr_Scope;

XVR_API Xvr_Scope *Xvr_pushScope(Xvr_Bucket **bucketHandle, Xvr_Scope *scope);
XVR_API Xvr_Scope *Xvr_popScope(Xvr_Scope *scope);

XVR_API Xvr_Scope *Xvr_deepCopyScope(Xvr_Bucket **bucketHandle,
                                     Xvr_Scope *scope);

XVR_API void Xvr_declareScope(Xvr_Scope *scope, Xvr_String *key,
                              Xvr_Value value);
XVR_API void Xvr_assignScope(Xvr_Scope *scope, Xvr_String *key,
                             Xvr_Value value);
XVR_API Xvr_Value Xvr_accessScope(Xvr_Scope *scope, Xvr_String *key);

XVR_API bool Xvr_isDeclaredScope(Xvr_Scope *scope, Xvr_String *key);

#endif // !XVR_SCOPE_H
