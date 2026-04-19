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

#include "core/ir/xvr_ir_generator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/ir/xvr_ir.h"
#include "xvr_literal.h"

struct Xvr_IRGenerator {
    Xvr_DiagnosticsPort* diagnostics;
    char* error;
    Xvr_IRModule* current_module;
};

static Xvr_IRType* map_literal_type_to_ir(Xvr_LiteralType literal_type) {
    switch (literal_type) {
    case XVR_LITERAL_NULL:
    case XVR_LITERAL_VOID:
        return Xvr_IRTypeCreateVoid();
    case XVR_LITERAL_BOOLEAN:
        return Xvr_IRTypeCreateInt(1);
    case XVR_LITERAL_INTEGER:
    case XVR_LITERAL_INT8:
    case XVR_LITERAL_INT16:
    case XVR_LITERAL_INT32:
    case XVR_LITERAL_INT64:
    case XVR_LITERAL_UINT8:
    case XVR_LITERAL_UINT16:
    case XVR_LITERAL_UINT32:
    case XVR_LITERAL_UINT64:
        return Xvr_IRTypeCreateInt(32);
    case XVR_LITERAL_FLOAT:
    case XVR_LITERAL_FLOAT16:
    case XVR_LITERAL_FLOAT32:
        return Xvr_IRTypeCreateFloat();
    case XVR_LITERAL_FLOAT64:
        return Xvr_IRTypeCreateDouble();
    case XVR_LITERAL_STRING:
        return Xvr_IRTypeCreatePointer(Xvr_IRTypeCreateInt(8));
    case XVR_LITERAL_ARRAY:
    case XVR_LITERAL_ARRAY_INTERMEDIATE:
        return Xvr_IRTypeCreatePointer(Xvr_IRTypeCreateInt(8));
    case XVR_LITERAL_DICTIONARY:
    case XVR_LITERAL_DICTIONARY_INTERMEDIATE:
        return Xvr_IRTypeCreatePointer(Xvr_IRTypeCreateInt(8));
    case XVR_LITERAL_FUNCTION:
    case XVR_LITERAL_FUNCTION_NATIVE:
    case XVR_LITERAL_FUNCTION_INTERMEDIATE:
        return Xvr_IRTypeCreatePointer(Xvr_IRTypeCreateInt(8));
    case XVR_LITERAL_IDENTIFIER:
        return Xvr_IRTypeCreatePointer(Xvr_IRTypeCreateInt(8));
    case XVR_LITERAL_TYPE:
    case XVR_LITERAL_TYPE_INTERMEDIATE:
        return Xvr_IRTypeCreatePointer(Xvr_IRTypeCreateInt(8));
    default:
        return Xvr_IRTypeCreateVoid();
    }
}

static Xvr_IROpcode map_opcode_to_ir(Xvr_Opcode opcode) {
    switch (opcode) {
    case XVR_OP_ADDITION:
        return XVR_IR_ADD;
    case XVR_OP_SUBTRACTION:
        return XVR_IR_SUB;
    case XVR_OP_MULTIPLICATION:
        return XVR_IR_MUL;
    case XVR_OP_DIVISION:
        return XVR_IR_DIV;
    case XVR_OP_MODULO:
        return XVR_IR_MOD;
    case XVR_OP_COMPARE_EQUAL:
        return XVR_IR_CMP_EQ;
    case XVR_OP_COMPARE_NOT_EQUAL:
        return XVR_IR_CMP_NE;
    case XVR_OP_COMPARE_LESS:
        return XVR_IR_CMP_LT;
    case XVR_OP_COMPARE_LESS_EQUAL:
        return XVR_IR_CMP_LE;
    case XVR_OP_COMPARE_GREATER:
        return XVR_IR_CMP_GT;
    case XVR_OP_COMPARE_GREATER_EQUAL:
        return XVR_IR_CMP_GE;
    case XVR_OP_AND:
        return XVR_IR_AND;
    case XVR_OP_OR:
        return XVR_IR_OR;
    default:
        return XVR_IR_NOP;
    }
}

static void translate_expression(Xvr_IRGenerator* gen, Xvr_ASTNode* node,
                                 Xvr_IRBasicBlock* block) {
    if (!node) {
        return;
    }

    switch (node->type) {
    case XVR_AST_NODE_LITERAL: {
        Xvr_NodeLiteral* lit = &node->atomic;
        Xvr_IRType* ir_type = map_literal_type_to_ir(lit->literal.type);
        char name[32];
        snprintf(name, sizeof(name), "const_%p", (void*)node);
        Xvr_IRValue* value = Xvr_IRValueCreate(ir_type, name);
        Xvr_IRBasicBlockAppendInstr(block, XVR_IR_LOAD, ir_type, &value, 1);
        break;
    }
    case XVR_AST_NODE_BINARY: {
        Xvr_NodeBinary* binary = &node->binary;
        Xvr_IRBasicBlock* left_block = block;
        translate_expression(gen, binary->left, left_block);
        Xvr_IRBasicBlock* right_block = block;
        translate_expression(gen, binary->right, right_block);
        Xvr_IROpcode op = map_opcode_to_ir(binary->opcode);
        Xvr_IRType* result_type = Xvr_IRTypeCreateInt(32);
        Xvr_IRBasicBlockAppendInstr(block, op, result_type, NULL, 0);
        break;
    }
    case XVR_AST_NODE_UNARY: {
        Xvr_NodeUnary* unary = &node->unary;
        translate_expression(gen, unary->child, block);
        break;
    }
    case XVR_AST_NODE_FN_CALL: {
        Xvr_NodeFnCall* call = &node->fnCall;
        if (call->arguments) {
            Xvr_NodeCompound* args = &call->arguments->compound;
            for (int i = 0; i < args->count; i++) {
                translate_expression(gen, &args->nodes[i], block);
            }
        }
        break;
    }
    default:
        break;
    }
}

static void translate_statement(Xvr_IRGenerator* gen, Xvr_ASTNode* node,
                                Xvr_IRFunction* func, Xvr_IRBasicBlock* block) {
    if (!node) {
        return;
    }

    switch (node->type) {
    case XVR_AST_NODE_VAR_DECL: {
        Xvr_NodeVarDecl* var_decl = &node->varDecl;
        Xvr_IRType* var_type = NULL;
        if (var_decl->typeLiteral.type == XVR_LITERAL_TYPE) {
            var_type =
                map_literal_type_to_ir(var_decl->typeLiteral.as.type.typeOf);
        } else {
            var_type = Xvr_IRTypeCreateInt(32);
        }
        char name[256];
        if (var_decl->identifier.as.identifier.ptr) {
            snprintf(name, sizeof(name), "%s",
                     var_decl->identifier.as.identifier.ptr->data);
        } else {
            snprintf(name, sizeof(name), "var_%p", (void*)node);
        }
        Xvr_IRBasicBlockAppendInstr(block, XVR_IR_ALLOCA, var_type, NULL, 0);
        if (var_decl->expression) {
            translate_expression(gen, var_decl->expression, block);
        }
        break;
    }
    case XVR_AST_NODE_FN_RETURN: {
        Xvr_NodeFnReturn* ret = &node->returns;
        if (ret->returns) {
            translate_expression(gen, ret->returns, block);
        }
        Xvr_IRType* void_type = Xvr_IRTypeCreateVoid();
        Xvr_IRBasicBlockAppendInstr(block, XVR_IR_RET, void_type, NULL, 0);
        break;
    }
    case XVR_AST_NODE_IF: {
        Xvr_NodeIf* if_node = &node->pathIf;
        Xvr_IRBasicBlock* then_block = Xvr_IRFunctionAddBlock(func, "if_then");
        translate_statement(gen, if_node->thenPath, func, then_block);
        if (if_node->elsePath) {
            Xvr_IRBasicBlock* else_block =
                Xvr_IRFunctionAddBlock(func, "if_else");
            translate_statement(gen, if_node->elsePath, func, else_block);
        }
        break;
    }
    case XVR_AST_NODE_WHILE: {
        Xvr_NodeWhile* while_node = &node->pathWhile;
        Xvr_IRBasicBlock* loop_block =
            Xvr_IRFunctionAddBlock(func, "while_body");
        translate_statement(gen, while_node->thenPath, func, loop_block);
        break;
    }
    case XVR_AST_NODE_FOR: {
        Xvr_NodeFor* for_node = &node->pathFor;
        Xvr_IRBasicBlock* for_block = Xvr_IRFunctionAddBlock(func, "for_body");
        translate_statement(gen, for_node->thenPath, func, for_block);
        break;
    }
    case XVR_AST_NODE_BREAK:
        break;
    case XVR_AST_NODE_CONTINUE:
        break;
    case XVR_AST_NODE_BLOCK: {
        Xvr_NodeBlock* block_node = &node->block;
        for (int i = 0; i < block_node->count; i++) {
            translate_statement(gen, &block_node->nodes[i], func, block);
        }
        break;
    }
    default:
        translate_expression(gen, node, block);
        break;
    }
}

static void translate_function(Xvr_IRGenerator* gen, Xvr_ASTNode* node) {
    if (node->type != XVR_AST_NODE_FN_DECL) {
        return;
    }

    Xvr_NodeFnDecl* fn_decl = &node->fnDecl;

    const char* fn_name = "anonymous";
    if (fn_decl->identifier.as.identifier.ptr) {
        fn_name = fn_decl->identifier.as.identifier.ptr->data;
    }

    Xvr_IRType* return_type = Xvr_IRTypeCreateVoid();
    if (fn_decl->returns) {
        if (fn_decl->returns->type == XVR_AST_NODE_LITERAL) {
            Xvr_Literal return_type_lit = fn_decl->returns->atomic.literal;
            if (return_type_lit.type == XVR_LITERAL_TYPE) {
                return_type =
                    map_literal_type_to_ir(return_type_lit.as.type.typeOf);
            }
        }
    }

    Xvr_IRType** param_types = NULL;
    size_t param_count = 0;

    if (fn_decl->arguments &&
        fn_decl->arguments->type == XVR_AST_NODE_COMPOUND) {
        Xvr_NodeCompound* params = &fn_decl->arguments->compound;
        param_count = (size_t)params->count;
        if (param_count > 0) {
            param_types = (Xvr_IRType**)calloc(param_count, sizeof(Xvr_IRType*));
            if (param_types) {
                for (size_t i = 0; i < param_count; i++) {
                    if (params->nodes[i].type == XVR_AST_NODE_VAR_DECL) {
                        Xvr_NodeVarDecl* param = &params->nodes[i].varDecl;
                        if (param->typeLiteral.type == XVR_LITERAL_TYPE) {
                            param_types[i] = map_literal_type_to_ir(
                                param->typeLiteral.as.type.typeOf);
                        } else {
                            param_types[i] = Xvr_IRTypeCreateInt(32);
                        }
                    }
                }
            }
        }
    }

    Xvr_IRFunction* func = Xvr_IRModuleAddFunction(
        gen->current_module, fn_name, return_type, param_types, param_count);

    if (param_types) {
        free(param_types);
    }

    if (!func) {
        gen->error = strdup("Failed to create IR function");
        return;
    }

    Xvr_IRBasicBlock* entry_block = Xvr_IRFunctionAddBlock(func, "entry");
    if (!entry_block) {
        gen->error = strdup("Failed to create entry block");
        return;
    }

    if (fn_decl->block && fn_decl->block->type == XVR_AST_NODE_BLOCK) {
        translate_statement(gen, fn_decl->block, func, entry_block);
    }
}

Xvr_IRGenerator* Xvr_IRGeneratorCreate(Xvr_DiagnosticsPort* diagnostics) {
    Xvr_IRGenerator* gen = (Xvr_IRGenerator*)calloc(1, sizeof(Xvr_IRGenerator));
    if (!gen) {
        return NULL;
    }
    gen->diagnostics = diagnostics;
    return gen;
}

void Xvr_IRGeneratorDestroy(Xvr_IRGenerator* gen) {
    if (!gen) {
        return;
    }
    if (gen->error) {
        free(gen->error);
    }
    if (gen->current_module) {
        Xvr_IRModuleDestroy(gen->current_module);
    }
    free(gen);
}

Xvr_IRModule* Xvr_IRGeneratorTranslate(Xvr_IRGenerator* gen,
                                       Xvr_ASTNode** ast_nodes,
                                       int node_count) {
    if (!gen || !ast_nodes || node_count <= 0) {
        if (gen) {
            gen->error = strdup("Invalid arguments to translate");
        }
        return NULL;
    }

    gen->current_module = Xvr_IRModuleCreate("main");
    if (!gen->current_module) {
        gen->error = strdup("Failed to create IR module");
        return NULL;
    }

    for (int i = 0; i < node_count; i++) {
        Xvr_ASTNode* node = ast_nodes[i];
        if (!node) {
            continue;
        }

        if (node->type == XVR_AST_NODE_FN_DECL) {
            translate_function(gen, node);
        } else if (node->type == XVR_AST_NODE_FN_COLLECTION) {
            Xvr_NodeFnCollection* collection = &node->fnCollection;
            for (int j = 0; j < collection->count; j++) {
                translate_function(gen, &collection->nodes[j]);
            }
        }
    }

    if (gen->error) {
        Xvr_IRModuleDestroy(gen->current_module);
        gen->current_module = NULL;
        return NULL;
    }

    Xvr_IRModule* result = gen->current_module;
    gen->current_module = NULL;
    return result;
}

bool Xvr_IRGeneratorHasError(const Xvr_IRGenerator* gen) {
    return gen && gen->error != NULL;
}

const char* Xvr_IRGeneratorGetError(const Xvr_IRGenerator* gen) {
    if (!gen) {
        return "NULL generator";
    }
    return gen->error ? gen->error : "";
}
