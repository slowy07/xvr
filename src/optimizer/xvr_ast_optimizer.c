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

#include "optimizer/xvr_ast_optimizer.h"

#include <stdlib.h>
#include <string.h>

#include "xvr_ast_node.h"
#include "xvr_literal.h"

struct Xvr_ASTOptimizer {
    Xvr_OptimizationLevel level;
    Xvr_ASTOptimizerPass* passes;
    int pass_count;
    int pass_capacity;
};

Xvr_ASTOptimizer* Xvr_ASTOptimizerCreate(void) {
    Xvr_ASTOptimizer* opt = calloc(1, sizeof(Xvr_ASTOptimizer));
    if (!opt) {
        return NULL;
    }
    opt->level = XVR_OPT_LEVEL_O2;
    opt->passes = NULL;
    opt->pass_count = 0;
    opt->pass_capacity = 16;
    return opt;
}

void Xvr_ASTOptimizerDestroy(Xvr_ASTOptimizer* opt) {
    if (!opt) {
        return;
    }
    if (opt->passes) {
        for (int i = 0; i < opt->pass_count; i++) {
            if (opt->passes[i].context) {
                free(opt->passes[i].context);
            }
        }
        free(opt->passes);
    }
    free(opt);
}

void Xvr_ASTOptimizerSetLevel(Xvr_ASTOptimizer* opt,
                              Xvr_OptimizationLevel level) {
    if (!opt) {
        return;
    }
    opt->level = level;
}

bool Xvr_ASTOptimizerAddPass(Xvr_ASTOptimizer* opt,
                             Xvr_ASTOptimizerPass* pass) {
    if (!opt || !pass) {
        return false;
    }
    if (!opt->passes || opt->pass_count >= opt->pass_capacity) {
        int new_cap = opt->pass_capacity > 0 ? opt->pass_capacity * 2 : 4;
        Xvr_ASTOptimizerPass* new_passes =
            realloc(opt->passes, new_cap * sizeof(Xvr_ASTOptimizerPass));
        if (!new_passes) {
            return false;
        }
        opt->passes = new_passes;
        opt->pass_capacity = new_cap;
    }
    opt->passes[opt->pass_count++] = *pass;
    return true;
}

Xvr_OptimizationLevel Xvr_OptimizationLevelFromInt(int level) {
    switch (level) {
    case 0:
        return XVR_OPT_LEVEL_NONE;
    case 1:
        return XVR_OPT_LEVEL_O1;
    case 2:
        return XVR_OPT_LEVEL_O2;
    case 3:
        return XVR_OPT_LEVEL_O3;
    default:
        return XVR_OPT_LEVEL_O2;
    }
}

static int compare_pass_priority(const void* a, const void* b) {
    const Xvr_ASTOptimizerPass* pa = (const Xvr_ASTOptimizerPass*)a;
    const Xvr_ASTOptimizerPass* pb = (const Xvr_ASTOptimizerPass*)b;
    return pa->priority - pb->priority;
}

Xvr_ASTOptimizerResult Xvr_ASTOptimizerRun(Xvr_ASTOptimizer* opt,
                                           Xvr_ASTNode** nodes,
                                           int node_count) {
    Xvr_ASTOptimizerResult result = {XVR_OPT_RESULT_SUCCESS, 0, NULL};

    if (!opt || !nodes || node_count == 0) {
        result.code = XVR_OPT_RESULT_ERROR;
        result.error_message = "invalid arguments";
        return result;
    }

    if (opt->level == XVR_OPT_LEVEL_NONE) {
        return result;
    }

    qsort(opt->passes, opt->pass_count, sizeof(Xvr_ASTOptimizerPass),
          compare_pass_priority);

    for (int i = 0; i < opt->pass_count; i++) {
        if (!opt->passes[i].enabled) {
            continue;
        }

        for (int j = 0; j < node_count; j++) {
            Xvr_ASTOptimizerResult pass_result =
                opt->passes[i].run(&nodes[j], opt->passes[i].context);
            if (pass_result.code == XVR_OPT_RESULT_ERROR) {
                result.code = XVR_OPT_RESULT_ERROR;
                result.error_message = pass_result.error_message;
                return result;
            }
            result.changes_made += pass_result.changes_made;
        }
    }

    if (result.changes_made == 0) {
        result.code = XVR_OPT_RESULT_NO_CHANGE;
    }

    return result;
}

typedef struct {
    int changes;
} Xvr_ConstantFoldingContext;

static bool is_integer_literal(Xvr_ASTNode* node) {
    if (!node || node->type != XVR_AST_NODE_LITERAL) {
        return false;
    }
    return node->atomic.literal.type == XVR_LITERAL_INTEGER ||
           node->atomic.literal.type == XVR_LITERAL_INT8 ||
           node->atomic.literal.type == XVR_LITERAL_INT16 ||
           node->atomic.literal.type == XVR_LITERAL_INT32 ||
           node->atomic.literal.type == XVR_LITERAL_INT64;
}

static bool is_bool_literal(Xvr_ASTNode* node) {
    if (!node || node->type != XVR_AST_NODE_LITERAL) {
        return false;
    }
    return node->atomic.literal.type == XVR_LITERAL_BOOLEAN;
}

static int64_t get_integer_value(Xvr_ASTNode* node) {
    if (!node || node->type != XVR_AST_NODE_LITERAL) {
        return 0;
    }
    return node->atomic.literal.as.integer;
}

static bool get_bool_value(Xvr_ASTNode* node) {
    if (!node || node->type != XVR_AST_NODE_LITERAL) {
        return false;
    }
    return node->atomic.literal.as.boolean;
}

static Xvr_ASTNode* fold_binary_arithmetic(Xvr_ASTNode* node) {
    if (!node || node->type != XVR_AST_NODE_BINARY) {
        return NULL;
    }

    if (!is_integer_literal(node->binary.left) ||
        !is_integer_literal(node->binary.right)) {
        return NULL;
    }

    int64_t left_val = get_integer_value(node->binary.left);
    int64_t right_val = get_integer_value(node->binary.right);
    int64_t result_val = 0;
    bool do_fold = false;

    switch (node->binary.opcode) {
    case 1:
        result_val = left_val + right_val;
        do_fold = true;
        break;
    case 2:
        result_val = left_val - right_val;
        do_fold = true;
        break;
    case 3:
        result_val = left_val * right_val;
        do_fold = true;
        break;
    case 4:
        if (right_val != 0) {
            result_val = left_val / right_val;
            do_fold = true;
        }
        break;
    case 5:
        if (right_val != 0) {
            result_val = left_val % right_val;
            do_fold = true;
        }
        break;
    default:
        break;
    }

    if (!do_fold) {
        return NULL;
    }

    Xvr_ASTNode* new_node = NULL;
    Xvr_Literal lit = {.as.integer = result_val,
                       .type = XVR_LITERAL_INTEGER,
                       0};
    Xvr_emitASTNodeLiteral(&new_node, lit);
    return new_node;
}

static Xvr_ASTOptimizerResult run_constant_folding(Xvr_ASTNode** node,
                                                   void* context) {
    (void)context;
    Xvr_ASTOptimizerResult result = {XVR_OPT_RESULT_SUCCESS, 0, NULL};

    if (!node || !*node) {
        return result;
    }

    Xvr_ASTNode* n = *node;

    if (n->type == XVR_AST_NODE_BINARY) {
        Xvr_ASTNode* folded = fold_binary_arithmetic(n);
        if (folded) {
            Xvr_freeASTNode(n);
            *node = folded;
            result.changes_made++;
        }
    }

    return result;
}

typedef struct {
    int changes;
} Xvr_DCEContext;

static bool has_side_effects_node(Xvr_ASTNode* node) {
    if (!node) {
        return false;
    }

    switch (node->type) {
    case XVR_AST_NODE_LITERAL:
        return false;
    case XVR_AST_NODE_UNARY:
        return has_side_effects_node(node->unary.child);
    case XVR_AST_NODE_BINARY:
        return has_side_effects_node(node->binary.left) ||
               has_side_effects_node(node->binary.right);
    case XVR_AST_NODE_VAR_DECL:
        return has_side_effects_node(node->varDecl.expression);
    case XVR_AST_NODE_FN_CALL:
        return true;
    case XVR_AST_NODE_BLOCK:
        if (node->block.nodes) {
            for (int i = 0; i < node->block.count; i++) {
                if (has_side_effects_node(&node->block.nodes[i])) {
                    return true;
                }
            }
        }
        return false;
    case XVR_AST_NODE_IF:
        if (has_side_effects_node(node->pathIf.condition)) return true;
        if (has_side_effects_node(node->pathIf.thenPath)) return true;
        if (has_side_effects_node(node->pathIf.elsePath)) return true;
        return false;
    case XVR_AST_NODE_WHILE:
    case XVR_AST_NODE_FOR:
        return true;
    case XVR_AST_NODE_BREAK:
    case XVR_AST_NODE_CONTINUE:
    case XVR_AST_NODE_PREFIX_INCREMENT:
    case XVR_AST_NODE_POSTFIX_INCREMENT:
    case XVR_AST_NODE_PREFIX_DECREMENT:
    case XVR_AST_NODE_POSTFIX_DECREMENT:
        return true;
    case XVR_AST_NODE_FN_RETURN:
        return has_side_effects_node(node->returns.returns);
    default:
        return false;
    }
}

static Xvr_ASTOptimizerResult run_dead_code_elimination(Xvr_ASTNode** node,
                                                        void* context) {
    (void)context;
    Xvr_ASTOptimizerResult result = {XVR_OPT_RESULT_SUCCESS, 0, NULL};

    if (!node || !*node) {
        return result;
    }

    Xvr_ASTNode* n = *node;

    if (n->type == XVR_AST_NODE_IF) {
        if (n->pathIf.condition && is_bool_literal(n->pathIf.condition)) {
            bool cond_value = get_bool_value(n->pathIf.condition);
            Xvr_ASTNode* keep =
                cond_value ? n->pathIf.thenPath : n->pathIf.elsePath;
            Xvr_ASTNode* discard =
                cond_value ? n->pathIf.elsePath : n->pathIf.thenPath;

            if (discard && !has_side_effects_node(discard)) {
                if (keep) {
                    Xvr_freeASTNode(n);
                    *node = keep;
                } else {
                    Xvr_emitASTNodeBlock(node);
                }
                result.changes_made++;
                return result;
            }
        }
    }

    return result;
}

bool Xvr_ASTOptimizerAddStandardPasses(Xvr_ASTOptimizer* opt) {
    if (!opt) {
        return false;
    }

    Xvr_ConstantFoldingContext* cf_ctx =
        calloc(1, sizeof(Xvr_ConstantFoldingContext));
    Xvr_ASTOptimizerPass cf_pass = {.type = XVR_PASS_CONSTANT_FOLDING,
                                    .name = "constant_folding",
                                    .run = run_constant_folding,
                                    .context = cf_ctx,
                                    .priority = 1,
                                    .enabled = true};
    Xvr_ASTOptimizerAddPass(opt, &cf_pass);

    Xvr_DCEContext* dce_ctx = calloc(1, sizeof(Xvr_DCEContext));
    Xvr_ASTOptimizerPass dce_pass = {.type = XVR_PASS_DEAD_CODE_ELIMINATION,
                                     .name = "dead_code_elimination",
                                     .run = run_dead_code_elimination,
                                     .context = dce_ctx,
                                     .priority = 2,
                                     .enabled = true};
    Xvr_ASTOptimizerAddPass(opt, &dce_pass);

    return true;
}
