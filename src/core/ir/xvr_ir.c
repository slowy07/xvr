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

#include "core/ir/xvr_ir.h"

#include <stdlib.h>
#include <string.h>

static size_t g_instruction_id = 0;

static void* xvr_ir_xmalloc(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        return NULL;
    }
    return ptr;
}

static void* xvr_ir_xcalloc(size_t nmemb, size_t size) {
    void* ptr = calloc(nmemb, size);
    if (!ptr) {
        return NULL;
    }
    return ptr;
}

static char* xvr_ir_strdup(const char* str, size_t max_len) {
    if (!str) {
        return NULL;
    }
    size_t len = 0;
    while (len < max_len && str[len] != '\0') {
        len++;
    }
    if (len >= max_len) {
        return NULL;
    }
    size_t alloc_size = len + 1;
    char* copy = xvr_ir_xmalloc(alloc_size);
    if (!copy) {
        return NULL;
    }
    if (alloc_size > 0) {
        memcpy(copy, str, alloc_size);
    }
    return copy;
}

void Xvr_IRTypeDestroy(Xvr_IRType* type) {
    if (!type) {
        return;
    }
    switch (type->kind) {
    case XVR_IR_TYPE_ARRAY:
        if (type->data.array.elem_type) {
            Xvr_IRTypeDestroy(type->data.array.elem_type);
        }
        break;
    case XVR_IR_TYPE_STRUCT:
        if (type->data.struct_type.elem_types) {
            for (size_t i = 0; i < type->data.struct_type.count; i++) {
                Xvr_IRTypeDestroy(type->data.struct_type.elem_types[i]);
            }
            free(type->data.struct_type.elem_types);
        }
        break;
    case XVR_IR_TYPE_FUNCTION:
        if (type->data.function.return_type) {
            Xvr_IRTypeDestroy(type->data.function.return_type);
        }
        if (type->data.function.param_types) {
            for (size_t i = 0; i < type->data.function.param_count; i++) {
                Xvr_IRTypeDestroy(type->data.function.param_types[i]);
            }
            free(type->data.function.param_types);
        }
        break;
    case XVR_IR_TYPE_POINTER:
        if (type->data.pointer_to) {
            Xvr_IRTypeDestroy(type->data.pointer_to);
        }
        break;
    default:
        break;
    }
    free(type);
}

void Xvr_IRValueDestroy(Xvr_IRValue* value) {
    if (!value) {
        return;
    }
    free((char*)value->name);
    free(value);
}

void Xvr_IRInstructionDestroy(Xvr_IRInstruction* instr) {
    if (!instr) {
        return;
    }
    if (instr->operands) {
        free(instr->operands);
    }
    free(instr);
}

void Xvr_IRBasicBlockDestroy(Xvr_IRBasicBlock* block) {
    if (!block) {
        return;
    }
    Xvr_IRInstruction* instr = block->instructions;
    while (instr) {
        Xvr_IRInstruction* next = instr->next;
        Xvr_IRInstructionDestroy(instr);
        instr = next;
    }
    free((char*)block->name);
    free(block);
}

void Xvr_IRFunctionDestroy(Xvr_IRFunction* func) {
    if (!func) {
        return;
    }
    free((char*)func->name);
    if (func->param_types) {
        for (size_t i = 0; i < func->param_count; i++) {
            Xvr_IRTypeDestroy(func->param_types[i]);
        }
        free(func->param_types);
    }
    if (func->return_type) {
        Xvr_IRTypeDestroy(func->return_type);
    }
    Xvr_IRBasicBlock* block = func->blocks;
    while (block) {
        Xvr_IRBasicBlock* next = block->next;
        Xvr_IRBasicBlockDestroy(block);
        block = next;
    }
    free(func);
}

void Xvr_IRModuleDestroy(Xvr_IRModule* module) {
    if (!module) {
        return;
    }
    free(module->name);
    if (module->functions) {
        for (size_t i = 0; i < module->function_count; i++) {
            Xvr_IRFunctionDestroy(module->functions[i]);
        }
        free(module->functions);
    }
    if (module->types) {
        for (size_t i = 0; i < module->type_count; i++) {
            Xvr_IRTypeDestroy(module->types[i]);
        }
        free(module->types);
    }
    free(module);
}

Xvr_IRModule* Xvr_IRModuleCreate(const char* name) {
    Xvr_IRModule* module = xvr_ir_xcalloc(1, sizeof(Xvr_IRModule));
    if (!module) {
        return NULL;
    }
    module->name = xvr_ir_strdup(name, 256);
    if (!module->name) {
        free(module);
        return NULL;
    }
    return module;
}

Xvr_IRFunction* Xvr_IRModuleAddFunction(Xvr_IRModule* module, const char* name,
                                        Xvr_IRType* return_type,
                                        Xvr_IRType** param_types,
                                        size_t param_count) {
    if (!module || !name) {
        return NULL;
    }
    Xvr_IRFunction* func = xvr_ir_xcalloc(1, sizeof(Xvr_IRFunction));
    if (!func) {
        return NULL;
    }
    func->name = xvr_ir_strdup(name, 256);
    if (!func->name) {
        free(func);
        return NULL;
    }
    func->return_type = return_type;
    func->param_count = param_count;
    if (param_count > 0 && param_types) {
        func->param_types = xvr_ir_xcalloc(param_count, sizeof(Xvr_IRType*));
        if (!func->param_types) {
            free((char*)func->name);
            free(func);
            return NULL;
        }
        for (size_t i = 0; i < param_count; i++) {
            func->param_types[i] = param_types[i];
        }
    } else {
        func->param_types = NULL;
    }
    Xvr_IRFunction** new_functions =
        realloc(module->functions,
                (module->function_count + 1) * sizeof(Xvr_IRFunction*));
    if (!new_functions) {
        for (size_t i = 0; i < param_count; i++) {
            Xvr_IRTypeDestroy(func->param_types[i]);
        }
        free(func->param_types);
        free((char*)func->name);
        free(func);
        return NULL;
    }
    module->functions = new_functions;
    module->functions[module->function_count] = func;
    module->function_count++;
    return func;
}

Xvr_IRBasicBlock* Xvr_IRFunctionAddBlock(Xvr_IRFunction* func,
                                         const char* name) {
    if (!func || !name) {
        return NULL;
    }
    Xvr_IRBasicBlock* block = xvr_ir_xcalloc(1, sizeof(Xvr_IRBasicBlock));
    if (!block) {
        return NULL;
    }
    block->name = xvr_ir_strdup(name, 256);
    if (!block->name) {
        free(block);
        return NULL;
    }
    if (!func->blocks) {
        func->blocks = block;
        func->last_block = block;
    } else {
        func->last_block->next = block;
        func->last_block = block;
    }
    func->block_count++;
    return block;
}

Xvr_IRInstruction* Xvr_IRBasicBlockAppendInstr(Xvr_IRBasicBlock* block,
                                               Xvr_IROpcode opcode,
                                               Xvr_IRType* result_type,
                                               Xvr_IRValue** operands,
                                               size_t operand_count) {
    if (!block) {
        return NULL;
    }
    Xvr_IRInstruction* instr = xvr_ir_xcalloc(1, sizeof(Xvr_IRInstruction));
    if (!instr) {
        return NULL;
    }
    instr->opcode = opcode;
    instr->result_type = result_type;
    instr->operand_count = operand_count;
    instr->id = g_instruction_id++;
    if (operand_count > 0 && operands) {
        instr->operands = xvr_ir_xcalloc(operand_count, sizeof(Xvr_IRValue*));
        if (!instr->operands) {
            free(instr);
            return NULL;
        }
        for (size_t i = 0; i < operand_count; i++) {
            instr->operands[i] = operands[i];
        }
    } else {
        instr->operands = NULL;
    }
    if (!block->instructions) {
        block->instructions = instr;
        block->last_instruction = instr;
    } else {
        block->last_instruction->next = instr;
        block->last_instruction = instr;
    }
    return instr;
}

Xvr_IRType* Xvr_IRTypeCreateVoid(void) {
    Xvr_IRType* type = xvr_ir_xcalloc(1, sizeof(Xvr_IRType));
    if (!type) {
        return NULL;
    }
    type->kind = XVR_IR_TYPE_VOID;
    return type;
}

Xvr_IRType* Xvr_IRTypeCreateInt(size_t bits) {
    Xvr_IRType* type = xvr_ir_xcalloc(1, sizeof(Xvr_IRType));
    if (!type) {
        return NULL;
    }
    switch (bits) {
    case 1:
        type->kind = XVR_IR_TYPE_INT1;
        break;
    case 8:
        type->kind = XVR_IR_TYPE_INT8;
        break;
    case 16:
        type->kind = XVR_IR_TYPE_INT16;
        break;
    case 32:
        type->kind = XVR_IR_TYPE_INT32;
        break;
    case 64:
        type->kind = XVR_IR_TYPE_INT64;
        break;
    default:
        type->kind = XVR_IR_TYPE_INT32;
        break;
    }
    return type;
}

Xvr_IRType* Xvr_IRTypeCreateFloat(void) {
    Xvr_IRType* type = xvr_ir_xcalloc(1, sizeof(Xvr_IRType));
    if (!type) {
        return NULL;
    }
    type->kind = XVR_IR_TYPE_FLOAT;
    return type;
}

Xvr_IRType* Xvr_IRTypeCreateDouble(void) {
    Xvr_IRType* type = xvr_ir_xcalloc(1, sizeof(Xvr_IRType));
    if (!type) {
        return NULL;
    }
    type->kind = XVR_IR_TYPE_DOUBLE;
    return type;
}

Xvr_IRType* Xvr_IRTypeCreatePointer(Xvr_IRType* elem_type) {
    if (!elem_type) {
        return NULL;
    }
    Xvr_IRType* type = xvr_ir_xcalloc(1, sizeof(Xvr_IRType));
    if (!type) {
        return NULL;
    }
    type->kind = XVR_IR_TYPE_POINTER;
    type->data.pointer_to = elem_type;
    return type;
}

Xvr_IRType* Xvr_IRTypeCreateArray(Xvr_IRType* elem_type, size_t count) {
    if (!elem_type) {
        return NULL;
    }
    Xvr_IRType* type = xvr_ir_xcalloc(1, sizeof(Xvr_IRType));
    if (!type) {
        return NULL;
    }
    type->kind = XVR_IR_TYPE_ARRAY;
    type->data.array.elem_type = elem_type;
    type->data.array.count = count;
    return type;
}

Xvr_IRType* Xvr_IRTypeCreateFunction(Xvr_IRType* return_type,
                                     Xvr_IRType** param_types,
                                     size_t param_count) {
    Xvr_IRType* type = xvr_ir_xcalloc(1, sizeof(Xvr_IRType));
    if (!type) {
        return NULL;
    }
    type->kind = XVR_IR_TYPE_FUNCTION;
    type->data.function.return_type = return_type;
    type->data.function.param_count = param_count;
    if (param_count > 0 && param_types) {
        type->data.function.param_types =
            xvr_ir_xcalloc(param_count, sizeof(Xvr_IRType*));
        if (!type->data.function.param_types) {
            free(type);
            return NULL;
        }
        for (size_t i = 0; i < param_count; i++) {
            type->data.function.param_types[i] = param_types[i];
        }
    } else {
        type->data.function.param_types = NULL;
    }
    return type;
}

Xvr_IRValue* Xvr_IRValueCreate(Xvr_IRType* type, const char* name) {
    Xvr_IRValue* value = xvr_ir_xcalloc(1, sizeof(Xvr_IRValue));
    if (!value) {
        return NULL;
    }
    value->type = type;
    value->name = xvr_ir_strdup(name, 256);
    if (!value->name) {
        free(value);
        return NULL;
    }
    value->llvm_value = NULL;
    return value;
}
