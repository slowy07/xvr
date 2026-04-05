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

#ifndef XVR_AST_OPTIMIZER_H
#define XVR_AST_OPTIMIZER_H

#include <stdbool.h>
#include <stddef.h>

#include "xvr_ast_node.h"

typedef struct Xvr_ASTOptimizer Xvr_ASTOptimizer;
typedef struct Xvr_ASTOptimizerPass Xvr_ASTOptimizerPass;
typedef struct Xvr_ASTOptimizerResult Xvr_ASTOptimizerResult;

typedef enum {
    XVR_OPT_LEVEL_NONE,
    XVR_OPT_LEVEL_O1,
    XVR_OPT_LEVEL_O2,
    XVR_OPT_LEVEL_O3,
    XVR_OPT_LEVEL_OS
} Xvr_OptimizationLevel;

typedef enum {
    XVR_PASS_CONSTANT_FOLDING,
    XVR_PASS_CONSTANT_PROPAGATION,
    XVR_PASS_DEAD_CODE_ELIMINATION,
    XVR_PASS_ALGEBRAIC_SIMPLIFICATION,
    XVR_PASS_STRENGTH_REDUCTION,
    XVR_PASS_FUNCTION_INLINING
} Xvr_ASTPassType;

typedef enum {
    XVR_OPT_RESULT_SUCCESS,
    XVR_OPT_RESULT_ERROR,
    XVR_OPT_RESULT_NO_CHANGE
} Xvr_OptResultCode;

struct Xvr_ASTOptimizerResult {
    Xvr_OptResultCode code;
    int changes_made;
    const char* error_message;
};

typedef Xvr_ASTOptimizerResult (*Xvr_ASTPassRunFn)(Xvr_ASTNode** node,
                                                   void* context);

struct Xvr_ASTOptimizerPass {
    Xvr_ASTPassType type;
    const char* name;
    Xvr_ASTPassRunFn run;
    void* context;
    int priority;
    bool enabled;
};

Xvr_ASTOptimizer* Xvr_ASTOptimizerCreate(void);
void Xvr_ASTOptimizerDestroy(Xvr_ASTOptimizer* opt);

void Xvr_ASTOptimizerSetLevel(Xvr_ASTOptimizer* opt,
                              Xvr_OptimizationLevel level);

bool Xvr_ASTOptimizerAddPass(Xvr_ASTOptimizer* opt, Xvr_ASTOptimizerPass* pass);

bool Xvr_ASTOptimizerAddStandardPasses(Xvr_ASTOptimizer* opt);

Xvr_ASTOptimizerResult Xvr_ASTOptimizerRun(Xvr_ASTOptimizer* opt,
                                           Xvr_ASTNode** nodes, int node_count);

Xvr_OptimizationLevel Xvr_OptimizationLevelFromInt(int level);

#endif
