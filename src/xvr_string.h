#ifndef XVR_STRING_H
#define XVR_STRING_H

#include "xvr_bucket.h"
#include "xvr_common.h"
#include "xvr_value.h"

#define XVR_STRING_MAX_LENGTH 1000

typedef struct Xvr_String { // 32 | 64 BITNESS
  enum Xvr_StringType {
    XVR_STRING_NODE,
    XVR_STRING_LEAF,
    XVR_STRING_NAME,
  } type; // 4  | 4

  unsigned int length;     // 4  | 4
  unsigned int refCount;   // 4  | 4
  unsigned int cachedHash; // 4 | 4

  int _padding; // 4  | 4

  union {
    struct {
      struct Xvr_String *left;  // 4  | 8
      struct Xvr_String *right; // 4  | 8
    } node;                     // 8  | 16

    struct {
      int _dummy;  // 4  | 4
      char data[]; //-  | -
    } leaf;

    struct {
      Xvr_ValueType type; // 4 | 4
      char data[];        // - | -
    } name;
  } as;       // 8  | 16
} Xvr_String; // 24 | 32

XVR_API Xvr_String *Xvr_createString(Xvr_Bucket **bucket, const char *cstring);
XVR_API Xvr_String *Xvr_createStringLength(Xvr_Bucket **bucket,
                                           const char *cstring, int length);
XVR_API Xvr_String *Xvr_createNameString(Xvr_Bucket **bucketHandle,
                                         const char *cname, Xvr_ValueType type);

XVR_API Xvr_String *Xvr_copyString(Xvr_Bucket **bucket, Xvr_String *str);
XVR_API Xvr_String *Xvr_deepCopyString(Xvr_Bucket **bucket, Xvr_String *str);

XVR_API Xvr_String *Xvr_concatStrings(Xvr_Bucket **bucket, Xvr_String *left,
                                      Xvr_String *right);

XVR_API void Xvr_freeString(Xvr_String *str);

XVR_API int Xvr_getStringLength(Xvr_String *str);
XVR_API int Xvr_getStringRefCount(Xvr_String *str);

XVR_API char *Xvr_getStringRawBuffer(Xvr_String *str);

XVR_API int Xvr_compareStrings(Xvr_String *left, Xvr_String *right);

XVR_API unsigned int Xvr_hashString(Xvr_String *str);

#endif // !XVR_STRING_H
