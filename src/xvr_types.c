#include "xvr_types.h"

static const char *build = __DATE__ " " __TIME__ "; dev branch";

const char *Xvr_private_version_build() { return build; }
