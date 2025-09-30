#ifndef XVR_H
#define XVR_H

// general utilities
#include "xvr_common.h"
#include "xvr_console_colors.h"

// basic structures
#include "xvr_array.h"
#include "xvr_bucket.h"
#include "xvr_stack.h"
#include "xvr_string.h"
#include "xvr_table.h"
#include "xvr_value.h"

// IR structures and other components
#include "xvr_ast.h"
#include "xvr_routine.h"

// pipeline
#include "xvr_bytecode.h"
#include "xvr_lexer.h"
#include "xvr_parser.h"
#include "xvr_vm.h"

#endif // !XVR_H
