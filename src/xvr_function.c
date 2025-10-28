#include "xvr_function.h"

#include "xvr_bucket.h"
#include "xvr_module.h"

Xvr_Function* Xvr_createModuleFunction(Xvr_Bucket** bucketHandle,
                                       Xvr_Module module) {
    Xvr_Function* proc =
        (Xvr_Function*)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Function));

    proc->type = XVR_FUNCTION_MODULE;
    proc->module.module = module;

    return proc;
}
