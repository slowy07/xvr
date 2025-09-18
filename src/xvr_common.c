#include "xvr_common.h"

static const char *build = __DATE__ " " __TIME__ "; incomplete dev branch";

const char *Xvr_private_version_build() { return build; }
