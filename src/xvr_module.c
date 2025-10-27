#include "xvr_module.h"

#include "xvr_console_colors.h"

static inline unsigned int readUnsignedInt(unsigned char** handle) {
    unsigned int i = *((unsigned int*)(*handle));
    (*handle) += 4;
    return i;
}

Xvr_Module Xvr_parseModule(unsigned char* ptr) {
    Xvr_Module module;

    module.scopePtr = NULL;
    module.code = ptr;

    readUnsignedInt(&ptr);

    module.jumpsCount = readUnsignedInt(&ptr);
    module.paramCount = readUnsignedInt(&ptr);
    module.dataCount = readUnsignedInt(&ptr);
    module.subsCount = readUnsignedInt(&ptr);

    module.codeAddr = readUnsignedInt(&ptr);

    if (module.jumpsCount) {
        module.jumpsAddr = readUnsignedInt(&ptr);
    }

    if (module.paramCount) {
        module.paramAddr = readUnsignedInt(&ptr);
    }

    if (module.dataCount) {
        module.dataAddr = readUnsignedInt(&ptr);
    }

    if (module.subsCount) {
        module.subsAddr = readUnsignedInt(&ptr);
    }

    return module;
}
