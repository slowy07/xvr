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

#include "xvr_routine.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_ast.h"
#include "xvr_console_colors.h"
#include "xvr_opcodes.h"
#include "xvr_string.h"
#include "xvr_value.h"

void* Xvr_private_resizeEscapeArray(Xvr_private_EscapeArray* ptr,
                                    unsigned int capacity) {
    if (capacity == 0) {
        free(ptr);
        return NULL;
    }

    unsigned int originalCapacity = ptr == NULL ? 0 : ptr->capacity;
    unsigned int orignalCount = ptr == NULL ? 0 : ptr->count;

    ptr = (Xvr_private_EscapeArray*)realloc(
        ptr, capacity * sizeof(Xvr_private_EscapeEntry_t) +
                 sizeof(Xvr_private_EscapeArray));

    if (ptr == NULL) {
        fprintf(stderr,
                XVR_CC_ERROR
                "ERROR: Failed to resize an escape array within 'Xvr_Routine' "
                "from %d to %d capacity\n" XVR_CC_RESET,
                (int)originalCapacity, (int)capacity);
        exit(-1);
    }

    ptr->capacity = capacity;
    ptr->count = orignalCount;

    return ptr;
}

static void expand(unsigned char** handle, unsigned int* capacity,
                   unsigned int* count, unsigned int amount) {
    if ((*count) + amount > (*capacity)) {
        while ((*count) + amount > (*capacity)) {
            (*capacity) = (*capacity) < 8 ? 8 : (*capacity) * 2;
        }
        (*handle) = realloc((*handle), (*capacity));

        if ((*handle) == NULL) {
            fprintf(stderr,
                    XVR_CC_ERROR
                    "ERROR: Failed to allocate %d space for a part of "
                    "'Xvr_Routine'\n" XVR_CC_RESET,
                    (int)(*capacity));
            exit(1);
        }
    }
}

static void emitByte(unsigned char** handle, unsigned int* capacity,
                     unsigned int* count, unsigned char byte) {
    expand(handle, capacity, count, 1);
    ((unsigned char*)(*handle))[(*count)++] = byte;
}

static void emitInt(unsigned char** handle, unsigned int* capacity,
                    unsigned int* count, unsigned int bytes) {
    char* ptr = (char*)&bytes;
    emitByte(handle, capacity, count, *(ptr++));
    emitByte(handle, capacity, count, *(ptr++));
    emitByte(handle, capacity, count, *(ptr++));
    emitByte(handle, capacity, count, *(ptr++));
}

static void emitFloat(unsigned char** handle, unsigned int* capacity,
                      unsigned int* count, float bytes) {
    char* ptr = (char*)&bytes;
    emitByte(handle, capacity, count, *(ptr++));
    emitByte(handle, capacity, count, *(ptr++));
    emitByte(handle, capacity, count, *(ptr++));
    emitByte(handle, capacity, count, *(ptr++));
}

// write instructions based on the AST types
#define EMIT_BYTE(rt, part, byte)                        \
    emitByte((&((*rt)->part)), &((*rt)->part##Capacity), \
             &((*rt)->part##Count), byte)
#define EMIT_INT(rt, part, bytes)                                              \
    emitInt((&((*rt)->part)), &((*rt)->part##Capacity), &((*rt)->part##Count), \
            bytes)
#define EMIT_FLOAT(rt, part, bytes)                       \
    emitFloat((&((*rt)->part)), &((*rt)->part##Capacity), \
              &((*rt)->part##Count), bytes)

#define SKIP_BYTE(rt, part) (EMIT_BYTE(rt, part, 0), ((*rt)->part##Count - 1))
#define SKIP_INT(rt, part) (EMIT_INT(rt, part, 0), ((*rt)->part##Count - 4))

#define OVERWRITE_INT(rt, part, addr, bytes) \
    emitInt((&((*rt)->part)), &((*rt)->part##Capacity), &(addr), bytes);

#define CURRENT_ADDRESS(rt, part) ((*rt)->part##Count)

static void emitToJumpTable(Xvr_Routine** rt, unsigned int startAddr) {
    EMIT_INT(rt, code, (*rt)->jumpsCount);  // mark the jump index in the code
    EMIT_INT(rt, jumps, startAddr);         // save address at the jump index
}

static unsigned int emitString(Xvr_Routine** rt, Xvr_String* str) {
    // 4-byte alignment
    unsigned int length = str->info.length + 1;
    if (length % 4 != 0) {
        length += 4 - (length % 4);  // ceil
    }

    // grab the current start address
    unsigned int startAddr = (*rt)->dataCount;

    // move the string into the data section
    expand((&((*rt)->data)), &((*rt)->dataCapacity), &((*rt)->dataCount),
           length);

    if (str->info.type == XVR_STRING_NODE) {
        char* buffer = Xvr_getStringRawBuffer(str);
        memcpy((*rt)->data + (*rt)->dataCount, buffer, str->info.length + 1);
        free(buffer);
    } else if (str->info.type == XVR_STRING_LEAF) {
        memcpy((*rt)->data + (*rt)->dataCount, str->leaf.data,
               str->info.length + 1);
    } else if (str->info.type == XVR_STRING_NAME) {
        memcpy((*rt)->data + (*rt)->dataCount, str->name.data,
               str->info.length + 1);
    }

    (*rt)->dataCount += length;

    // mark the jump position
    emitToJumpTable(rt, startAddr);

    return 1;
}

static unsigned int writeRoutineCode(Xvr_Routine** rt, Xvr_Ast* ast);

static unsigned int writeInstructionValue(Xvr_Routine** rt, Xvr_AstValue ast) {
    EMIT_BYTE(rt, code, XVR_OPCODE_READ);
    EMIT_BYTE(rt, code, ast.value.type);

    // emit the raw value based on the type
    if (XVR_VALUE_IS_NULL(ast.value)) {
        // NOTHING - null's type data is enough

        // 4-byte alignment
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);
    } else if (XVR_VALUE_IS_BOOLEAN(ast.value)) {
        EMIT_BYTE(rt, code, XVR_VALUE_AS_BOOLEAN(ast.value));

        // 4-byte alignment
        EMIT_BYTE(rt, code, 0);
    } else if (XVR_VALUE_IS_INTEGER(ast.value)) {
        // 4-byte alignment
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);

        EMIT_INT(rt, code, XVR_VALUE_AS_INTEGER(ast.value));
    } else if (XVR_VALUE_IS_FLOAT(ast.value)) {
        // 4-byte alignment
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);

        EMIT_FLOAT(rt, code, XVR_VALUE_AS_FLOAT(ast.value));
    } else if (XVR_VALUE_IS_STRING(ast.value)) {
        // 4-byte alignment
        EMIT_BYTE(rt, code, XVR_STRING_LEAF);
        EMIT_BYTE(rt, code, 0);

        return emitString(rt, XVR_VALUE_AS_STRING(ast.value));
    } else {
        fprintf(
            stderr, XVR_CC_ERROR
            "ERROR: Invalid AST type found: Unknown value type\n" XVR_CC_RESET);
        exit(-1);
    }

    return 1;
}

static unsigned int writeInstructionUnary(Xvr_Routine** rt, Xvr_AstUnary ast) {
    // working with a stack means the child gets placed first
    unsigned int result = writeRoutineCode(rt, ast.child);

    if (ast.flag == XVR_AST_FLAG_NEGATE) {
        EMIT_BYTE(rt, code, XVR_OPCODE_NEGATE);

        // 4-byte alignment
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);
    } else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST unary flag found\n" XVR_CC_RESET);
        exit(-1);
    }

    return result;
}

static unsigned int writeInstructionBinary(Xvr_Routine** rt,
                                           Xvr_AstBinary ast) {
    // left, then right, then the binary's operation
    writeRoutineCode(rt, ast.left);
    writeRoutineCode(rt, ast.right);

    if (ast.flag == XVR_AST_FLAG_ADD) {
        EMIT_BYTE(rt, code, XVR_OPCODE_ADD);
    } else if (ast.flag == XVR_AST_FLAG_SUBTRACT) {
        EMIT_BYTE(rt, code, XVR_OPCODE_SUBTRACT);
    } else if (ast.flag == XVR_AST_FLAG_MULTIPLY) {
        EMIT_BYTE(rt, code, XVR_OPCODE_MULTIPLY);
    } else if (ast.flag == XVR_AST_FLAG_DIVIDE) {
        EMIT_BYTE(rt, code, XVR_OPCODE_DIVIDE);
    } else if (ast.flag == XVR_AST_FLAG_MODULO) {
        EMIT_BYTE(rt, code, XVR_OPCODE_MODULO);
    }

    else if (ast.flag == XVR_AST_FLAG_CONCAT) {
        EMIT_BYTE(rt, code, XVR_OPCODE_CONCAT);
    } else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST binary flag found\n" XVR_CC_RESET);
        exit(-1);
    }

    EMIT_BYTE(rt, code, XVR_OPCODE_PASS);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);

    return 1;
}

static unsigned int writeInstructionBinaryShortCircuit(
    Xvr_Routine** rt, Xvr_AstBinaryShortCircuit ast) {
    writeRoutineCode(rt, ast.left);

    EMIT_BYTE(rt, code, XVR_OPCODE_DUPLICATE);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);

    if (ast.flag == XVR_AST_FLAG_AND) {
        EMIT_BYTE(rt, code, XVR_OPCODE_JUMP);
        EMIT_BYTE(rt, code, XVR_OP_PARAM_JUMP_RELATIVE);
        EMIT_BYTE(rt, code, XVR_OP_PARAM_JUMP_IF_FALSE);
        EMIT_BYTE(rt, code, 0);
    }

    else if (ast.flag == XVR_AST_FLAG_OR) {
        EMIT_BYTE(rt, code, XVR_OPCODE_JUMP);
        EMIT_BYTE(rt, code, XVR_OP_PARAM_JUMP_RELATIVE);
        EMIT_BYTE(rt, code, XVR_OP_PARAM_JUMP_IF_TRUE);
        EMIT_BYTE(rt, code, 0);
    } else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: invalid AST binary short circuit flag "
                "found\n" XVR_CC_RESET);
        exit(-1);
    }

    unsigned int paramAddr = SKIP_INT(rt, code);

    EMIT_BYTE(rt, code, XVR_OPCODE_ELIMINATE);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);

    writeRoutineCode(rt, ast.right);
    OVERWRITE_INT(rt, code, paramAddr,
                  CURRENT_ADDRESS(rt, code) - (paramAddr + 4));

    return 1;
}

static unsigned int writeInstructionCompare(Xvr_Routine** rt,
                                            Xvr_AstCompare ast) {
    writeRoutineCode(rt, ast.left);
    writeRoutineCode(rt, ast.right);

    if (ast.flag == XVR_AST_FLAG_COMPARE_EQUAL) {
        EMIT_BYTE(rt, code, XVR_OPCODE_COMPARE_EQUAL);
    } else if (ast.flag == XVR_AST_FLAG_COMPARE_NOT) {
        EMIT_BYTE(rt, code, XVR_OPCODE_COMPARE_EQUAL);
        EMIT_BYTE(rt, code, XVR_OPCODE_NEGATE);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);

        return 1;
    } else if (ast.flag == XVR_AST_FLAG_COMPARE_LESS) {
        EMIT_BYTE(rt, code, XVR_OPCODE_COMPARE_LESS);
    } else if (ast.flag == XVR_AST_FLAG_COMPARE_LESS_EQUAL) {
        EMIT_BYTE(rt, code, XVR_OPCODE_COMPARE_LESS_EQUAL);
    } else if (ast.flag == XVR_AST_FLAG_COMPARE_GREATER) {
        EMIT_BYTE(rt, code, XVR_OPCODE_COMPARE_GREATER);
    } else if (ast.flag == XVR_AST_FLAG_COMPARE_GREATER_EQUAL) {
        EMIT_BYTE(rt, code, XVR_OPCODE_COMPARE_GREATER_EQUAL);
    }

    else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST compare flag found\n" XVR_CC_RESET);
        exit(-1);
    }

    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);

    return 1;
}

static unsigned int writeInstructionGroup(Xvr_Routine** rt, Xvr_AstGroup ast) {
    return writeRoutineCode(rt, ast.child);
}

static unsigned int writeInstructionCompound(Xvr_Routine** rt,
                                             Xvr_AstCompound ast) {
    unsigned int result = writeRoutineCode(rt, ast.child);

    if (ast.flag == XVR_AST_FLAG_COMPOUND_ARRAY) {
        EMIT_BYTE(rt, code, XVR_OPCODE_READ);
        EMIT_BYTE(rt, code, XVR_VALUE_ARRAY);

        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);

        EMIT_INT(rt, code, result);

        return 1;
    }

    if (ast.flag == XVR_AST_FLAG_COMPOUND_TABLE) {
        EMIT_BYTE(rt, code, XVR_OPCODE_READ);
        EMIT_BYTE(rt, code, XVR_VALUE_TABLE);

        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);

        EMIT_INT(rt, code, result);

        return 1;

    } else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST compound flag found\n" XVR_CC_RESET);
        exit(-1);
        return 0;
    }
}

static unsigned int writeInstructionAggregate(Xvr_Routine** rt,
                                              Xvr_AstAggregate ast) {
    unsigned int result = 0;

    result += writeRoutineCode(rt, ast.left);
    result += writeRoutineCode(rt, ast.right);

    if (ast.flag == XVR_AST_FLAG_COLLECTION) {
        return result;
    } else if (ast.flag == XVR_AST_FLAG_PAIR) {
        return result;
    } else if (ast.flag == XVR_AST_FLAG_INDEX) {
        EMIT_BYTE(rt, code, XVR_OPCODE_INDEX);
        EMIT_BYTE(rt, code, result);

        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);

        return 1;  // leaves only 1 value on the stack
    } else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST aggregate flag found\n" XVR_CC_RESET);
        exit(-1);
        return 0;
    }
}

static unsigned int writeInstructionAssert(Xvr_Routine** rt,
                                           Xvr_AstAssert ast) {
    writeRoutineCode(rt, ast.child);
    writeRoutineCode(rt, ast.message);

    EMIT_BYTE(rt, code, XVR_OPCODE_ASSERT);

    EMIT_BYTE(rt, code, ast.message != NULL ? 2 : 1);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);

    return 0;
}

static unsigned int writeInstructionIfThenElse(Xvr_Routine** rt,
                                               Xvr_AstIfThenElse ast) {
    writeRoutineCode(rt, ast.condBranch);

    EMIT_BYTE(rt, code, XVR_OPCODE_JUMP);
    EMIT_BYTE(rt, code, XVR_OP_PARAM_JUMP_RELATIVE);
    EMIT_BYTE(rt, code, XVR_OP_PARAM_JUMP_IF_FALSE);
    EMIT_BYTE(rt, code, 0);

    unsigned int thenParamAddr = SKIP_INT(rt, code);

    writeRoutineCode(rt, ast.thenBranch);

    if (ast.elseBranch != NULL) {
        EMIT_BYTE(rt, code, XVR_OPCODE_JUMP);
        EMIT_BYTE(rt, code, XVR_OP_PARAM_JUMP_RELATIVE);
        EMIT_BYTE(rt, code, XVR_OP_PARAM_JUMP_ALWAYS);
        EMIT_BYTE(rt, code, 0);

        unsigned int elseParamAddr = SKIP_INT(rt, code);

        OVERWRITE_INT(rt, code, thenParamAddr,
                      CURRENT_ADDRESS(rt, code) - (thenParamAddr + 4));

        writeRoutineCode(rt, ast.elseBranch);

        OVERWRITE_INT(rt, code, elseParamAddr,
                      CURRENT_ADDRESS(rt, code) - (elseParamAddr + 4));
    }

    else {
        OVERWRITE_INT(rt, code, thenParamAddr,
                      CURRENT_ADDRESS(rt, code) - (thenParamAddr + 4));
    }

    return 0;
}

static unsigned int writeInstructionWhileThen(Xvr_Routine** rt,
                                              Xvr_AstWhileThen ast) {
    unsigned int beginAddr = CURRENT_ADDRESS(rt, code);

    writeRoutineCode(rt, ast.condBranch);

    EMIT_BYTE(rt, code, XVR_OPCODE_JUMP);
    EMIT_BYTE(rt, code, XVR_OP_PARAM_JUMP_RELATIVE);
    EMIT_BYTE(rt, code, XVR_OP_PARAM_JUMP_IF_FALSE);
    EMIT_BYTE(rt, code, 0);

    unsigned int paramAddr = SKIP_INT(rt, code);

    writeRoutineCode(rt, ast.thenBranch);

    EMIT_BYTE(rt, code, XVR_OPCODE_JUMP);
    EMIT_BYTE(rt, code, XVR_OP_PARAM_JUMP_RELATIVE);
    EMIT_BYTE(rt, code, XVR_OP_PARAM_JUMP_ALWAYS);
    EMIT_BYTE(rt, code, 0);

    EMIT_INT(rt, code, beginAddr - (CURRENT_ADDRESS(rt, code) + 4));

    OVERWRITE_INT(rt, code, paramAddr,
                  CURRENT_ADDRESS(rt, code) - (paramAddr + 4));

    while ((*rt)->breakEscapes->count > 0) {
        unsigned int addr =
            (*rt)->breakEscapes->data[(*rt)->breakEscapes->count - 1].addr;
        unsigned int depth =
            (*rt)->breakEscapes->data[(*rt)->breakEscapes->count - 1].depth;

        unsigned int diff = depth - (*rt)->currentScopeDepth;
        OVERWRITE_INT(rt, code, addr, CURRENT_ADDRESS(rt, code) - (addr + 8));
        OVERWRITE_INT(rt, code, addr, diff);

        (*rt)->breakEscapes->count--;
    }

    while ((*rt)->continueEscapes->count > 0) {
        unsigned int addr =
            (*rt)
                ->continueEscapes->data[(*rt)->continueEscapes->count - 1]
                .addr;
        unsigned int depth =
            (*rt)
                ->continueEscapes->data[(*rt)->continueEscapes->count - 1]
                .depth;

        unsigned int diff = depth - (*rt)->currentScopeDepth;

        OVERWRITE_INT(rt, code, addr, addr - (CURRENT_ADDRESS(rt, code) + 8));
        OVERWRITE_INT(rt, code, diff, diff);

        (*rt)->continueEscapes->count--;
    }

    return 0;
}

static unsigned int writeInstructionBreak(Xvr_Routine** rt, Xvr_AstBreak ast) {
    (void)ast;

    EMIT_BYTE(rt, code, XVR_OPCODE_ESCAPE);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);

    unsigned int addr = SKIP_INT(rt, code);
    (void)SKIP_INT(rt, code);

    if ((*rt)->breakEscapes->capacity <= (*rt)->breakEscapes->count) {
        (*rt)->breakEscapes = Xvr_private_resizeEscapeArray(
            (*rt)->breakEscapes,
            (*rt)->breakEscapes->capacity * XVR_ESCAPE_EXPANSION_RATE);
    }

    (*rt)->breakEscapes->data[(*rt)->breakEscapes->count++] =
        (Xvr_private_EscapeEntry_t){.addr = addr,
                                    .depth = (*rt)->currentScopeDepth};

    return 0;
}

static unsigned int writeInstructionContinue(Xvr_Routine** rt,
                                             Xvr_AstContinue ast) {
    (void)ast;

    EMIT_BYTE(rt, code, XVR_OPCODE_ESCAPE);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);

    unsigned int addr = SKIP_INT(rt, code);
    (void)SKIP_INT(rt, code);

    if ((*rt)->continueEscapes->capacity <= (*rt)->continueEscapes->count) {
        (*rt)->continueEscapes = Xvr_private_resizeEscapeArray(
            (*rt)->continueEscapes,
            (*rt)->continueEscapes->capacity * XVR_ESCAPE_EXPANSION_RATE);
    }

    (*rt)->continueEscapes->data[(*rt)->continueEscapes->count++] =
        (Xvr_private_EscapeEntry_t){.addr = addr,
                                    .depth = (*rt)->currentScopeDepth};
    return 0;
}

static unsigned int writeInstructionPrint(Xvr_Routine** rt, Xvr_AstPrint ast) {
    // the thing to print
    writeRoutineCode(rt, ast.child);

    // output the print opcode
    EMIT_BYTE(rt, code, XVR_OPCODE_PRINT);

    // 4-byte alignment
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);

    return 0;
}

static unsigned int writeInstructionVarDeclare(Xvr_Routine** rt,
                                               Xvr_AstVarDeclare ast) {
    writeRoutineCode(rt, ast.expr);

    EMIT_BYTE(rt, code, XVR_OPCODE_DECLARE);
    EMIT_BYTE(rt, code, Xvr_getNameStringVarType(ast.name));
    EMIT_BYTE(rt, code, ast.name->info.length);
    EMIT_BYTE(rt, code, Xvr_getNameStringVarConstant(ast.name) ? 1 : 0);

    emitString(rt, ast.name);

    return 0;
}

static unsigned int writeInstructionAssign(Xvr_Routine** rt,
                                           Xvr_AstVarAssign ast) {
    unsigned int result = 0;
    switch (ast.expr->type) {
    case XVR_AST_BLOCK:
    case XVR_AST_AGGREGATE:
    case XVR_AST_ASSERT:
    case XVR_AST_PRINT:
    case XVR_AST_VAR_DECLARE:
        fprintf(stderr, XVR_CC_ERROR
                "COMPILER ERROR: Invalid AST type found: Malformed "
                "assignment\n" XVR_CC_RESET);
        (*rt)->panic = true;
        return 0;

    default:
        break;
    }

    if (ast.target->type == XVR_AST_VALUE &&
        XVR_VALUE_IS_STRING(ast.target->value.value) &&
        XVR_VALUE_AS_STRING(ast.target->value.value)->info.type ==
            XVR_STRING_NAME) {
        Xvr_String* target = XVR_VALUE_AS_STRING(ast.target->value.value);

        EMIT_BYTE(rt, code, XVR_OPCODE_READ);
        EMIT_BYTE(rt, code, XVR_VALUE_STRING);
        EMIT_BYTE(rt, code, XVR_STRING_NAME);
        EMIT_BYTE(rt, code, target->info.length);

        emitString(rt, target);
    } else if (ast.target->type == XVR_AST_AGGREGATE &&
               ast.target->aggregate.flag == XVR_AST_FLAG_INDEX) {
        writeRoutineCode(rt, ast.target->aggregate.left);
        writeRoutineCode(rt, ast.target->aggregate.right);
        writeRoutineCode(rt, ast.expr);

        EMIT_BYTE(rt, code, XVR_OPCODE_ASSIGN_COMPOUND);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);
        return 0;
    } else {
        fprintf(stderr,
                XVR_CC_ERROR "COMPILER ERROR: TODO at %s %d\n" XVR_CC_RESET,
                __FILE__, __LINE__);
        (*rt)->panic = true;
        return 0;
    }

    if (ast.flag == XVR_AST_FLAG_ASSIGN) {
        result += writeRoutineCode(rt, ast.expr);

        EMIT_BYTE(rt, code, XVR_OPCODE_ASSIGN);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);
    } else if (ast.flag == XVR_AST_FLAG_ADD_ASSIGN) {
        EMIT_BYTE(rt, code, XVR_OPCODE_DUPLICATE);
        EMIT_BYTE(rt, code, XVR_OPCODE_ACCESS);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);

        result += writeRoutineCode(rt, ast.expr);

        EMIT_BYTE(rt, code, XVR_OPCODE_ADD);
        EMIT_BYTE(rt, code, XVR_OPCODE_ASSIGN);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);
    } else if (ast.flag == XVR_AST_FLAG_SUBTRACT_ASSIGN) {
        EMIT_BYTE(rt, code, XVR_OPCODE_DUPLICATE);
        EMIT_BYTE(rt, code, XVR_OPCODE_ACCESS);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);

        result += writeRoutineCode(rt, ast.expr);

        EMIT_BYTE(rt, code, XVR_OPCODE_SUBTRACT);
        EMIT_BYTE(rt, code, XVR_OPCODE_ASSIGN);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);
    } else if (ast.flag == XVR_AST_FLAG_MULTIPLY_ASSIGN) {
        EMIT_BYTE(rt, code, XVR_OPCODE_DUPLICATE);
        EMIT_BYTE(rt, code, XVR_OPCODE_ACCESS);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);

        result += writeRoutineCode(rt, ast.expr);

        EMIT_BYTE(rt, code, XVR_OPCODE_MULTIPLY);
        EMIT_BYTE(rt, code, XVR_OPCODE_ASSIGN);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);
    } else if (ast.flag == XVR_AST_FLAG_DIVIDE_ASSIGN) {
        EMIT_BYTE(rt, code, XVR_OPCODE_DUPLICATE);
        EMIT_BYTE(rt, code, XVR_OPCODE_ACCESS);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);

        result += writeRoutineCode(rt, ast.expr);

        EMIT_BYTE(rt, code, XVR_OPCODE_DIVIDE);
        EMIT_BYTE(rt, code, XVR_OPCODE_ASSIGN);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);
    } else if (ast.flag == XVR_AST_FLAG_MODULO_ASSIGN) {
        EMIT_BYTE(rt, code, XVR_OPCODE_DUPLICATE);
        EMIT_BYTE(rt, code, XVR_OPCODE_ACCESS);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);

        result += writeRoutineCode(rt, ast.expr);

        EMIT_BYTE(rt, code, XVR_OPCODE_MODULO);
        EMIT_BYTE(rt, code, XVR_OPCODE_ASSIGN);
        EMIT_BYTE(rt, code, 0);
        EMIT_BYTE(rt, code, 0);
    }

    else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST assign flag found\n" XVR_CC_RESET);
        exit(-1);
    }

    return result;
}

static unsigned int writeInstructionAccess(Xvr_Routine** rt,
                                           Xvr_AstVarAccess ast) {
    if (!(ast.child->type == XVR_AST_VALUE &&
          XVR_VALUE_IS_STRING(ast.child->value.value) &&
          XVR_VALUE_AS_STRING(ast.child->value.value)->info.type ==
              XVR_STRING_NAME)) {
        fprintf(stderr, XVR_CC_ERROR
                "COMPILER ERROR: Found a non-name-string in a value node when "
                "trying to write access\n" XVR_CC_RESET);
        exit(-1);
    }

    Xvr_String* name = XVR_VALUE_AS_STRING(ast.child->value.value);

    EMIT_BYTE(rt, code, XVR_OPCODE_READ);
    EMIT_BYTE(rt, code, XVR_VALUE_STRING);
    EMIT_BYTE(rt, code, XVR_STRING_NAME);
    EMIT_BYTE(rt, code, name->info.length);

    emitString(rt, name);

    EMIT_BYTE(rt, code, XVR_OPCODE_ACCESS);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);
    EMIT_BYTE(rt, code, 0);

    return 1;
}

// routine structure
//  static void writeRoutineParam(Xvr_Routine* rt) {
//  	//
//  }

static unsigned int writeRoutineCode(Xvr_Routine** rt, Xvr_Ast* ast) {
    if (ast == NULL) {
        return 0;
    }

    // if an error occured, just exit
    if (rt == NULL || (*rt) == NULL || (*rt)->panic) {
        return 0;
    }

    unsigned int result = 0;

    // determine how to write each instruction based on the Ast
    switch (ast->type) {
    case XVR_AST_BLOCK:
        if (ast->block.innerScope) {
            EMIT_BYTE(rt, code, XVR_OPCODE_SCOPE_PUSH);
            EMIT_BYTE(rt, code, 0);
            EMIT_BYTE(rt, code, 0);
            EMIT_BYTE(rt, code, 0);

            (*rt)->currentScopeDepth++;
        }

        result += writeRoutineCode(rt, ast->block.child);
        result += writeRoutineCode(rt, ast->block.next);

        if (ast->block.innerScope) {
            EMIT_BYTE(rt, code, XVR_OPCODE_SCOPE_POP);
            EMIT_BYTE(rt, code, 0);
            EMIT_BYTE(rt, code, 0);
            EMIT_BYTE(rt, code, 0);

            (*rt)->currentScopeDepth--;
        }
        break;

    case XVR_AST_VALUE:
        result += writeInstructionValue(rt, ast->value);
        break;

    case XVR_AST_UNARY:
        result += writeInstructionUnary(rt, ast->unary);
        break;

    case XVR_AST_BINARY:
        result += writeInstructionBinary(rt, ast->binary);
        break;

    case XVR_AST_BINARY_SHORT_CIRCUIT:
        result +=
            writeInstructionBinaryShortCircuit(rt, ast->binaryShortCircuit);
        break;

    case XVR_AST_COMPARE:
        result += writeInstructionCompare(rt, ast->compare);
        break;

    case XVR_AST_GROUP:
        result += writeInstructionGroup(rt, ast->group);
        break;

    case XVR_AST_COMPOUND:
        result += writeInstructionCompound(rt, ast->compound);
        break;

    case XVR_AST_AGGREGATE:
        result += writeInstructionAggregate(rt, ast->aggregate);
        break;

    case XVR_AST_ASSERT:
        result += writeInstructionAssert(rt, ast->assert);
        break;

    case XVR_AST_IF_THEN_ELSE:
        result += writeInstructionIfThenElse(rt, ast->ifThenElse);
        break;

    case XVR_AST_WHILE_THEN:
        result += writeInstructionWhileThen(rt, ast->whileThen);
        break;

    case XVR_AST_BREAK:
        result += writeInstructionBreak(rt, ast->breakPoint);
        break;

    case XVR_AST_CONTINUE:
        result += writeInstructionContinue(rt, ast->continuePoint);
        break;

    case XVR_AST_PRINT:
        result += writeInstructionPrint(rt, ast->print);
        break;

    case XVR_AST_VAR_DECLARE:
        result += writeInstructionVarDeclare(rt, ast->varDeclare);
        break;

    case XVR_AST_VAR_ASSIGN:
        result += writeInstructionAssign(rt, ast->varAssign);
        break;

    case XVR_AST_VAR_ACCESS:
        result += writeInstructionAccess(rt, ast->varAccess);
        break;

    case XVR_AST_PASS:
        break;

    case XVR_AST_ERROR:
        fprintf(stderr, XVR_CC_ERROR
                "COMPILER ERROR: Invalid AST type found: Unknown "
                "'error'\n" XVR_CC_RESET);
        (*rt)->panic = true;
        break;

    case XVR_AST_END:
        fprintf(stderr, XVR_CC_ERROR
                "COMPILER ERROR: Invalid AST type found: Unknown "
                "'end'\n" XVR_CC_RESET);
        (*rt)->panic = true;
        break;
    }

    return result;
}

static void* writeRoutine(Xvr_Routine* rt, Xvr_Ast* ast) {
    writeRoutineCode(&rt, ast);
    EMIT_BYTE(&rt, code, XVR_OPCODE_RETURN);  // temp terminator
    EMIT_BYTE(&rt, code, 0);                  // 4-byte alignment
    EMIT_BYTE(&rt, code, 0);
    EMIT_BYTE(&rt, code, 0);

    if (rt->panic) {
        return NULL;
    }

    unsigned char* buffer = NULL;
    unsigned int capacity = 0, count = 0;
    int codeAddr = 0;
    int jumpsAddr = 0;
    int dataAddr = 0;

    emitInt(&buffer, &capacity, &count, 0);  // total size (overwritten later)
    emitInt(&buffer, &capacity, &count, rt->paramCount);  // param size
    emitInt(&buffer, &capacity, &count, rt->jumpsCount);  // jumps size
    emitInt(&buffer, &capacity, &count, rt->dataCount);   // data size
    emitInt(&buffer, &capacity, &count, rt->subsCount);   // routine size

    // generate blank spaces, cache their positions in the *Addr variables (for
    // storing the start positions)
    if (rt->paramCount > 0) {
        // paramAddr = count;
        emitInt(&buffer, &capacity, &count, 0);  // params
    }
    if (rt->codeCount > 0) {
        codeAddr = count;
        emitInt(&buffer, &capacity, &count, 0);  // code
    }
    if (rt->jumpsCount > 0) {
        jumpsAddr = count;
        emitInt(&buffer, &capacity, &count, 0);  // jumps
    }
    if (rt->dataCount > 0) {
        dataAddr = count;
        emitInt(&buffer, &capacity, &count, 0);  // data
    }
    if (rt->subsCount > 0) {
        // subsAddr = count;
        emitInt(&buffer, &capacity, &count, 0);  // subs
    }

    // append various parts to the buffer
    // TODO: param region

    if (rt->codeCount > 0) {
        expand(&buffer, &capacity, &count, rt->codeCount);
        memcpy((buffer + count), rt->code, rt->codeCount);

        *((int*)(buffer + codeAddr)) = count;
        count += rt->codeCount;
    }

    if (rt->jumpsCount > 0) {
        expand(&buffer, &capacity, &count, rt->jumpsCount);
        memcpy((buffer + count), rt->jumps, rt->jumpsCount);

        *((int*)(buffer + jumpsAddr)) = count;
        count += rt->jumpsCount;
    }

    if (rt->dataCount > 0) {
        expand(&buffer, &capacity, &count, rt->dataCount);
        memcpy((buffer + count), rt->data, rt->dataCount);

        *((int*)(buffer + dataAddr)) = count;
        count += rt->dataCount;
    }

    // TODO: subs region

    // finally, record the total size within the header, and return the result
    *((int*)buffer) = count;

    return buffer;
}

// exposed functions
void* Xvr_compileRoutine(Xvr_Ast* ast) {
    // setup
    Xvr_Routine rt;

    rt.param = NULL;
    rt.paramCapacity = 0;
    rt.paramCount = 0;

    rt.code = NULL;
    rt.codeCapacity = 0;
    rt.codeCount = 0;

    rt.jumps = NULL;
    rt.jumpsCapacity = 0;
    rt.jumpsCount = 0;

    rt.data = NULL;
    rt.dataCapacity = 0;
    rt.dataCount = 0;

    rt.subs = NULL;
    rt.subsCapacity = 0;
    rt.subsCount = 0;

    rt.currentScopeDepth = 0;
    rt.breakEscapes =
        Xvr_private_resizeEscapeArray(NULL, XVR_ESCAPE_INITIAL_CAPACITY);
    rt.continueEscapes =
        Xvr_private_resizeEscapeArray(NULL, XVR_ESCAPE_INITIAL_CAPACITY);

    rt.panic = false;

    // build
    void* buffer = writeRoutine(&rt, ast);

    Xvr_private_resizeEscapeArray(rt.breakEscapes, 0);
    Xvr_private_resizeEscapeArray(rt.continueEscapes, 0);

    // cleanup the temp object
    free(rt.param);
    free(rt.code);
    free(rt.jumps);
    free(rt.data);
    free(rt.subs);

    return buffer;
}
