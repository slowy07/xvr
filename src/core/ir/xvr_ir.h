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

/**
 * @brief internal IR (intermediate representation) type and structure
 * definitions
 *
 * This is the compiler-owned IR layer that sits between the AST and LLVM
 * codegen. It provides backend independence by allowing different adapters to
 * translate from this internal IR to various targets (LLVM, WebAssembly, etc.)
 *
 * memory management:
 *   - all structures are heap-allocated
 *   - Xvr_IRModuleDestroy recursively frees all contained objects
 *   - caller is responsible for managing operand lifetime
 *
 * threading:
 *   - not thread-safe, external synchronization required
 *
 */

#ifndef XVR_CORE_IR_H
#define XVR_CORE_IR_H

#include <stdbool.h>
#include <stddef.h>

typedef struct Xvr_IRType Xvr_IRType;

typedef enum Xvr_IROpcode {
    XVR_IR_NOP,
    XVR_IR_LOAD,
    XVR_IR_STORE,
    XVR_IR_ALLOCA,
    XVR_IR_ADD,
    XVR_IR_SUB,
    XVR_IR_MUL,
    XVR_IR_DIV,
    XVR_IR_MOD,
    XVR_IR_AND,
    XVR_IR_OR,
    XVR_IR_XOR,
    XVR_IR_SHL,
    XVR_IR_SHR,
    XVR_IR_CMP_EQ,
    XVR_IR_CMP_NE,
    XVR_IR_CMP_LT,
    XVR_IR_CMP_LE,
    XVR_IR_CMP_GT,
    XVR_IR_CMP_GE,
    XVR_IR_CALL,
    XVR_IR_RET,
    XVR_IR_BR,
    XVR_IR_COND_BR,
    XVR_IR_PHI,
    XVR_IR_CAST,
    XVR_IR_GEP,
    XVR_IR_EXTRACT,
    XVR_IR_INSERT,
} Xvr_IROpcode;

typedef enum Xvr_IRTypeKind {
    XVR_IR_TYPE_VOID,
    XVR_IR_TYPE_INT1,
    XVR_IR_TYPE_INT8,
    XVR_IR_TYPE_INT16,
    XVR_IR_TYPE_INT32,
    XVR_IR_TYPE_INT64,
    XVR_IR_TYPE_FLOAT,
    XVR_IR_TYPE_DOUBLE,
    XVR_IR_TYPE_POINTER,
    XVR_IR_TYPE_ARRAY,
    XVR_IR_TYPE_STRUCT,
    XVR_IR_TYPE_FUNCTION,
} Xvr_IRTypeKind;

typedef struct Xvr_IRType {
    Xvr_IRTypeKind kind;
    union {
        struct {
            Xvr_IRType* elem_type;
            size_t count;
        } array;
        struct {
            Xvr_IRType** elem_types;
            size_t count;
        } struct_type;
        struct {
            Xvr_IRType* return_type;
            Xvr_IRType** param_types;
            size_t param_count;
        } function;
        Xvr_IRType* pointer_to;
    } data;
} Xvr_IRType;

typedef struct Xvr_IRValue {
    Xvr_IRType* type;
    const char* name;
    void* llvm_value;
} Xvr_IRValue;

typedef struct Xvr_IRInstruction {
    Xvr_IROpcode opcode;
    Xvr_IRType* result_type;
    Xvr_IRValue** operands;
    size_t operand_count;
    struct Xvr_IRInstruction* next;
    size_t id;
} Xvr_IRInstruction;

typedef struct Xvr_IRBasicBlock {
    const char* name;
    Xvr_IRInstruction* instructions;
    Xvr_IRInstruction* last_instruction;
    struct Xvr_IRBasicBlock* next;
} Xvr_IRBasicBlock;

typedef struct Xvr_IRFunction {
    const char* name;
    Xvr_IRType* return_type;
    Xvr_IRType** param_types;
    size_t param_count;
    Xvr_IRBasicBlock* blocks;
    Xvr_IRBasicBlock* last_block;
    size_t block_count;
} Xvr_IRFunction;

typedef struct Xvr_IRModule {
    char* name;
    Xvr_IRFunction** functions;
    size_t function_count;
    Xvr_IRType** types;
    size_t type_count;
} Xvr_IRModule;

Xvr_IRModule* Xvr_IRModuleCreate(const char* name);
void Xvr_IRModuleDestroy(Xvr_IRModule* module);
Xvr_IRFunction* Xvr_IRModuleAddFunction(Xvr_IRModule* module, const char* name,
                                        Xvr_IRType* return_type,
                                        Xvr_IRType** param_types,
                                        size_t param_count);
Xvr_IRBasicBlock* Xvr_IRFunctionAddBlock(Xvr_IRFunction* func,
                                         const char* name);
Xvr_IRInstruction* Xvr_IRBasicBlockAppendInstr(Xvr_IRBasicBlock* block,
                                               Xvr_IROpcode opcode,
                                               Xvr_IRType* result_type,
                                               Xvr_IRValue** operands,
                                               size_t operand_count);
Xvr_IRType* Xvr_IRTypeCreateVoid(void);
Xvr_IRType* Xvr_IRTypeCreateInt(size_t bits);
Xvr_IRType* Xvr_IRTypeCreateFloat(void);
Xvr_IRType* Xvr_IRTypeCreateDouble(void);
Xvr_IRType* Xvr_IRTypeCreatePointer(Xvr_IRType* elem_type);
Xvr_IRType* Xvr_IRTypeCreateArray(Xvr_IRType* elem_type, size_t count);
Xvr_IRType* Xvr_IRTypeCreateFunction(Xvr_IRType* return_type,
                                     Xvr_IRType** param_types,
                                     size_t param_count);
Xvr_IRValue* Xvr_IRValueCreate(Xvr_IRType* type, const char* name);

#endif
