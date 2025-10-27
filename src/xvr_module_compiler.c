#include "xvr_module_compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xvr_ast.h"
#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_module.h"
#include "xvr_opcodes.h"
#include "xvr_string.h"
#include "xvr_value.h"

static bool checkForChaining(Xvr_Ast* ptr) {
    // BUGFIX
    if (ptr == NULL) {
        return false;
    }

    if (ptr->type == XVR_AST_VAR_ASSIGN) {
        return true;
    }

    if (ptr->type == XVR_AST_UNARY) {
        if (ptr->unary.flag >= XVR_AST_FLAG_PREFIX_INCREMENT &&
            ptr->unary.flag <= XVR_AST_FLAG_POSTFIX_DECREMENT) {
            return true;
        }
    }

    return false;
}

// escapes
void* Xvr_private_resizeEscapeArray(Xvr_private_EscapeArray* ptr,
                                    unsigned int capacity) {
    // if you're freeing everything, just return
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
                "ERROR: Failed to resize an escape array within "
                "'Xvr_ModuleCompiler' from %d to %d capacity\n" XVR_CC_RESET,
                (int)originalCapacity, (int)capacity);
        exit(-1);
    }

    ptr->capacity = capacity;
    ptr->count = orignalCount;

    return ptr;
}

// writing utils
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
                    "'Xvr_ModuleCompiler'\n" XVR_CC_RESET,
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

// curry writing utils
#define EMIT_BYTE(mb, part, byte)                        \
    emitByte((&((*mb)->part)), &((*mb)->part##Capacity), \
             &((*mb)->part##Count), byte)
#define EMIT_INT(mb, part, bytes)                                              \
    emitInt((&((*mb)->part)), &((*mb)->part##Capacity), &((*mb)->part##Count), \
            bytes)
#define EMIT_FLOAT(mb, part, bytes)                       \
    emitFloat((&((*mb)->part)), &((*mb)->part##Capacity), \
              &((*mb)->part##Count), bytes)

// skip bytes, but return the address
#define SKIP_BYTE(mb, part) (EMIT_BYTE(mb, part, 0), ((*mb)->part##Count - 1))
#define SKIP_INT(mb, part) (EMIT_INT(mb, part, 0), ((*mb)->part##Count - 4))

// overwrite a pre-existing position
#define OVERWRITE_INT(mb, part, addr, bytes) \
    emitInt((&((*mb)->part)), &((*mb)->part##Capacity), &(addr), bytes);

// simply get the address (always an integer)
#define CURRENT_ADDRESS(mb, part) ((*mb)->part##Count)

static void emitToJumpTable(Xvr_ModuleCompiler** mb, unsigned int startAddr) {
    EMIT_INT(mb, code, (*mb)->jumpsCount);  // mark the jump index in the code
    EMIT_INT(mb, jumps, startAddr);         // save address at the jump index
}

static unsigned int emitString(Xvr_ModuleCompiler** mb, Xvr_String* str) {
    // 4-byte alignment
    unsigned int length = str->info.length + 1;
    if (length % 4 != 0) {
        length += 4 - (length % 4);  // ceil
    }

    // grab the current start address
    unsigned int startAddr = (*mb)->dataCount;

    // move the string into the data section
    expand((&((*mb)->data)), &((*mb)->dataCapacity), &((*mb)->dataCount),
           length);

    if (str->info.type == XVR_STRING_NODE) {
        char* buffer = Xvr_getStringRawBuffer(str);
        memcpy((*mb)->data + (*mb)->dataCount, buffer, str->info.length + 1);
        free(buffer);
    } else if (str->info.type == XVR_STRING_LEAF) {
        memcpy((*mb)->data + (*mb)->dataCount, str->leaf.data,
               str->info.length + 1);
    } else if (str->info.type == XVR_STRING_NAME) {
        memcpy((*mb)->data + (*mb)->dataCount, str->name.data,
               str->info.length + 1);
    }

    (*mb)->dataCount += length;

    // mark the jump position
    emitToJumpTable(mb, startAddr);

    return 1;
}

static unsigned int writeModuleCompilerCode(
    Xvr_ModuleCompiler** mb, Xvr_Ast* ast);  // forward declare for recursion
static unsigned int writeInstructionAssign(
    Xvr_ModuleCompiler** mb, Xvr_AstVarAssign ast,
    bool
        chainedAssignment);  // forward declare for chaining of var declarations

static unsigned int writeInstructionValue(Xvr_ModuleCompiler** mb,
                                          Xvr_AstValue ast) {
    EMIT_BYTE(mb, code, XVR_OPCODE_READ);
    EMIT_BYTE(mb, code, ast.value.type);

    // emit the raw value based on the type
    if (XVR_VALUE_IS_NULL(ast.value)) {
        // NOTHING - null's type data is enough

        // 4-byte alignment
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);
    } else if (XVR_VALUE_IS_BOOLEAN(ast.value)) {
        EMIT_BYTE(mb, code, XVR_VALUE_AS_BOOLEAN(ast.value));

        // 4-byte alignment
        EMIT_BYTE(mb, code, 0);
    } else if (XVR_VALUE_IS_INTEGER(ast.value)) {
        // 4-byte alignment
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        EMIT_INT(mb, code, XVR_VALUE_AS_INTEGER(ast.value));
    } else if (XVR_VALUE_IS_FLOAT(ast.value)) {
        // 4-byte alignment
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        EMIT_FLOAT(mb, code, XVR_VALUE_AS_FLOAT(ast.value));
    } else if (XVR_VALUE_IS_STRING(ast.value)) {
        // 4-byte alignment
        EMIT_BYTE(mb, code, XVR_STRING_LEAF);  // normal string
        EMIT_BYTE(mb, code, 0);                // can't store the length

        return emitString(mb, XVR_VALUE_AS_STRING(ast.value));
    } else {
        fprintf(
            stderr, XVR_CC_ERROR
            "ERROR: Invalid AST type found: Unknown value type\n" XVR_CC_RESET);
        exit(-1);
    }

    return 1;
}

static unsigned int writeInstructionUnary(Xvr_ModuleCompiler** mb,
                                          Xvr_AstUnary ast) {
    unsigned int result = 0;

    if (ast.flag == XVR_AST_FLAG_NEGATE) {
        result = writeModuleCompilerCode(mb, ast.child);

        EMIT_BYTE(mb, code, XVR_OPCODE_NEGATE);

        // 4-byte alignment
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);
    }

    else if (ast.flag == XVR_AST_FLAG_PREFIX_INCREMENT ||
             ast.flag == XVR_AST_FLAG_PREFIX_DECREMENT) {
        // read the var name onto the stack
        Xvr_String* name = XVR_VALUE_AS_STRING(ast.child->value.value);

        EMIT_BYTE(mb, code, XVR_OPCODE_READ);
        EMIT_BYTE(mb, code, XVR_VALUE_STRING);
        EMIT_BYTE(mb, code, XVR_STRING_NAME);
        EMIT_BYTE(mb, code, name->info.length);  // store the length (max 255)

        emitString(mb, name);

        // duplicate the var name, then get the value
        EMIT_BYTE(mb, code, XVR_OPCODE_DUPLICATE);
        EMIT_BYTE(mb, code, XVR_OPCODE_ACCESS);  // squeezed
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        // read the integer '1'
        EMIT_BYTE(mb, code, XVR_OPCODE_READ);
        EMIT_BYTE(mb, code, XVR_VALUE_INTEGER);
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        EMIT_INT(mb, code, 1);
        EMIT_BYTE(mb, code,
                  ast.flag == XVR_AST_FLAG_PREFIX_INCREMENT
                      ? XVR_OPCODE_ADD
                      : XVR_OPCODE_SUBTRACT);
        EMIT_BYTE(mb, code, XVR_OPCODE_ASSIGN);  // squeezed
        EMIT_BYTE(mb, code, 1);
        EMIT_BYTE(mb, code, 0);

        // leaves one value on the stack
        result = 1;
    }

    else if (ast.flag == XVR_AST_FLAG_POSTFIX_INCREMENT ||
             ast.flag == XVR_AST_FLAG_POSTFIX_DECREMENT) {  // NOTE: ditto
        // read the var name onto the stack
        Xvr_String* name = XVR_VALUE_AS_STRING(ast.child->value.value);

        EMIT_BYTE(mb, code, XVR_OPCODE_READ);
        EMIT_BYTE(mb, code, XVR_VALUE_STRING);
        EMIT_BYTE(mb, code, XVR_STRING_NAME);
        EMIT_BYTE(mb, code, name->info.length);  // store the length (max 255)

        emitString(mb, name);

        // access the value (postfix++ and postfix--)
        EMIT_BYTE(mb, code, XVR_OPCODE_ACCESS);
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        // read the var name onto the stack (again)
        name = XVR_VALUE_AS_STRING(ast.child->value.value);

        EMIT_BYTE(mb, code, XVR_OPCODE_READ);
        EMIT_BYTE(mb, code, XVR_VALUE_STRING);
        EMIT_BYTE(mb, code, XVR_STRING_NAME);
        EMIT_BYTE(mb, code, name->info.length);  // store the length (max 255)

        emitString(mb, name);

        // duplicate the var name, then get the value
        EMIT_BYTE(mb, code, XVR_OPCODE_DUPLICATE);
        EMIT_BYTE(mb, code, XVR_OPCODE_ACCESS);  // squeezed
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        // read the integer '1'
        EMIT_BYTE(mb, code, XVR_OPCODE_READ);
        EMIT_BYTE(mb, code, XVR_VALUE_INTEGER);
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        EMIT_INT(mb, code, 1);

        // add (or subtract) the two values, then assign (pops the second
        // duplicate)
        EMIT_BYTE(mb, code,
                  ast.flag == XVR_AST_FLAG_POSTFIX_INCREMENT
                      ? XVR_OPCODE_ADD
                      : XVR_OPCODE_SUBTRACT);
        EMIT_BYTE(mb, code, XVR_OPCODE_ASSIGN);  // squeezed
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        // leaves one value on the stack
        result = 1;
    }

    else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST unary flag found\n" XVR_CC_RESET);
        exit(-1);
    }

    return result;
}

static unsigned int writeInstructionBinary(Xvr_ModuleCompiler** mb,
                                           Xvr_AstBinary ast) {
    // left, then right, then the binary's operation
    writeModuleCompilerCode(mb, ast.left);
    writeModuleCompilerCode(mb, ast.right);

    if (ast.flag == XVR_AST_FLAG_ADD) {
        EMIT_BYTE(mb, code, XVR_OPCODE_ADD);
    } else if (ast.flag == XVR_AST_FLAG_SUBTRACT) {
        EMIT_BYTE(mb, code, XVR_OPCODE_SUBTRACT);
    } else if (ast.flag == XVR_AST_FLAG_MULTIPLY) {
        EMIT_BYTE(mb, code, XVR_OPCODE_MULTIPLY);
    } else if (ast.flag == XVR_AST_FLAG_DIVIDE) {
        EMIT_BYTE(mb, code, XVR_OPCODE_DIVIDE);
    } else if (ast.flag == XVR_AST_FLAG_MODULO) {
        EMIT_BYTE(mb, code, XVR_OPCODE_MODULO);
    }

    else if (ast.flag == XVR_AST_FLAG_CONCAT) {
        EMIT_BYTE(mb, code, XVR_OPCODE_CONCAT);
    } else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST binary flag found\n" XVR_CC_RESET);
        exit(-1);
    }

    // 4-byte alignment
    EMIT_BYTE(mb, code, XVR_OPCODE_PASS);  // checked in combined assignments
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);

    return 1;  // leaves only 1 value on the stack
}

static unsigned int writeInstructionBinaryShortCircuit(
    Xvr_ModuleCompiler** mb, Xvr_AstBinaryShortCircuit ast) {
    // lhs
    writeModuleCompilerCode(mb, ast.left);

    // duplicate the top (so the lhs can be 'returned' by this expression, if
    // needed)
    EMIT_BYTE(mb, code, XVR_OPCODE_DUPLICATE);
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);

    // && return the first falsy operand, or the last operand
    if (ast.flag == XVR_AST_FLAG_AND) {
        EMIT_BYTE(mb, code, XVR_OPCODE_JUMP);
        EMIT_BYTE(mb, code, XVR_OP_PARAM_JUMP_RELATIVE);
        EMIT_BYTE(mb, code, XVR_OP_PARAM_JUMP_IF_FALSE);
        EMIT_BYTE(mb, code, 0);
    }

    // || return the first truthy operand, or the last operand
    else if (ast.flag == XVR_AST_FLAG_OR) {
        EMIT_BYTE(mb, code, XVR_OPCODE_JUMP);
        EMIT_BYTE(mb, code, XVR_OP_PARAM_JUMP_RELATIVE);
        EMIT_BYTE(mb, code, XVR_OP_PARAM_JUMP_IF_TRUE);
        EMIT_BYTE(mb, code, 0);
    }

    else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST binary short circuit flag "
                "found\n" XVR_CC_RESET);
        exit(-1);
    }

    // parameter address
    unsigned int paramAddr =
        SKIP_INT(mb, code);  // parameter to be written later

    // if the lhs value isn't needed, pop it
    EMIT_BYTE(mb, code, XVR_OPCODE_ELIMINATE);
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);

    // rhs
    writeModuleCompilerCode(mb, ast.right);

    // set the parameter
    OVERWRITE_INT(mb, code, paramAddr,
                  CURRENT_ADDRESS(mb, code) - (paramAddr + 4));

    return 1;  // leaves only 1 value on the stack
}

static unsigned int writeInstructionCompare(Xvr_ModuleCompiler** mb,
                                            Xvr_AstCompare ast) {
    // left, then right, then the compare's operation
    writeModuleCompilerCode(mb, ast.left);
    writeModuleCompilerCode(mb, ast.right);

    if (ast.flag == XVR_AST_FLAG_COMPARE_EQUAL) {
        EMIT_BYTE(mb, code, XVR_OPCODE_COMPARE_EQUAL);
    } else if (ast.flag == XVR_AST_FLAG_COMPARE_NOT) {
        EMIT_BYTE(mb, code, XVR_OPCODE_COMPARE_EQUAL);
        EMIT_BYTE(mb, code, XVR_OPCODE_NEGATE);  // squeezed
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        return 1;
    } else if (ast.flag == XVR_AST_FLAG_COMPARE_LESS) {
        EMIT_BYTE(mb, code, XVR_OPCODE_COMPARE_LESS);
    } else if (ast.flag == XVR_AST_FLAG_COMPARE_LESS_EQUAL) {
        EMIT_BYTE(mb, code, XVR_OPCODE_COMPARE_LESS_EQUAL);
    } else if (ast.flag == XVR_AST_FLAG_COMPARE_GREATER) {
        EMIT_BYTE(mb, code, XVR_OPCODE_COMPARE_GREATER);
    } else if (ast.flag == XVR_AST_FLAG_COMPARE_GREATER_EQUAL) {
        EMIT_BYTE(mb, code, XVR_OPCODE_COMPARE_GREATER_EQUAL);
    }

    else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST compare flag found\n" XVR_CC_RESET);
        exit(-1);
    }

    // 4-byte alignment (covers most cases)
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);

    return 1;  // leaves only 1 value on the stack
}

static unsigned int writeInstructionGroup(Xvr_ModuleCompiler** mb,
                                          Xvr_AstGroup ast) {
    // not certain what this leaves
    return writeModuleCompilerCode(mb, ast.child);
}

static unsigned int writeInstructionCompound(Xvr_ModuleCompiler** mb,
                                             Xvr_AstCompound ast) {
    unsigned int result = writeModuleCompilerCode(mb, ast.child);

    if (ast.flag == XVR_AST_FLAG_COMPOUND_ARRAY) {
        // signal how many values to read in as array elements
        EMIT_BYTE(mb, code, XVR_OPCODE_READ);
        EMIT_BYTE(mb, code, XVR_VALUE_ARRAY);

        // 4-byte alignment
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        // how many elements
        EMIT_INT(mb, code, result);

        return 1;  // leaves only 1 value on the stack
    }
    if (ast.flag == XVR_AST_FLAG_COMPOUND_TABLE) {
        // signal how many values to read in as table elements
        EMIT_BYTE(mb, code, XVR_OPCODE_READ);
        EMIT_BYTE(mb, code, XVR_VALUE_TABLE);

        // 4-byte alignment
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        // how many elements
        EMIT_INT(mb, code, result);

        return 1;  // leaves only 1 value on the stack
    } else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST compound flag found\n" XVR_CC_RESET);
        exit(-1);
        return 0;
    }
}

static unsigned int writeInstructionAggregate(Xvr_ModuleCompiler** mb,
                                              Xvr_AstAggregate ast) {
    unsigned int result = 0;

    // left, then right
    result += writeModuleCompilerCode(mb, ast.left);
    result += writeModuleCompilerCode(mb, ast.right);

    if (ast.flag == XVR_AST_FLAG_COLLECTION) {
        // collections are handled above
        return result;
    } else if (ast.flag == XVR_AST_FLAG_PAIR) {
        // pairs are handled above
        return result;
    } else if (ast.flag == XVR_AST_FLAG_INDEX) {
        // value[index, length]
        EMIT_BYTE(mb, code, XVR_OPCODE_INDEX);
        EMIT_BYTE(mb, code, result);

        // 4-byte alignment
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        return 1;  // leaves only 1 value on the stack
    } else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST aggregate flag found\n" XVR_CC_RESET);
        exit(-1);
        return 0;
    }
}

static unsigned int writeInstructionAssert(Xvr_ModuleCompiler** mb,
                                           Xvr_AstAssert ast) {
    // the thing to print
    writeModuleCompilerCode(mb, ast.child);
    writeModuleCompilerCode(mb, ast.message);

    // output the print opcode
    EMIT_BYTE(mb, code, XVR_OPCODE_ASSERT);

    // 4-byte alignment
    EMIT_BYTE(mb, code, ast.message != NULL ? 2 : 1);  // arg count
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);

    return 0;
}

static unsigned int writeInstructionIfThenElse(Xvr_ModuleCompiler** mb,
                                               Xvr_AstIfThenElse ast) {
    // cond-branch
    writeModuleCompilerCode(mb, ast.condBranch);

    // emit the jump word (opcode, type, condition, padding)
    EMIT_BYTE(mb, code, XVR_OPCODE_JUMP);
    EMIT_BYTE(mb, code, XVR_OP_PARAM_JUMP_RELATIVE);
    EMIT_BYTE(mb, code, XVR_OP_PARAM_JUMP_IF_FALSE);
    EMIT_BYTE(mb, code, 0);

    unsigned int thenParamAddr =
        SKIP_INT(mb, code);  // parameter to be written later

    // emit then-branch
    writeModuleCompilerCode(mb, ast.thenBranch);

    if (ast.elseBranch != NULL) {
        // emit the jump-to-end (opcode, type, condition, padding)
        EMIT_BYTE(mb, code, XVR_OPCODE_JUMP);
        EMIT_BYTE(mb, code, XVR_OP_PARAM_JUMP_RELATIVE);
        EMIT_BYTE(mb, code, XVR_OP_PARAM_JUMP_ALWAYS);
        EMIT_BYTE(mb, code, 0);

        unsigned int elseParamAddr =
            SKIP_INT(mb, code);  // parameter to be written later

        // specify the starting position for the else branch
        OVERWRITE_INT(mb, code, thenParamAddr,
                      CURRENT_ADDRESS(mb, code) - (thenParamAddr + 4));

        // emit the else branch
        writeModuleCompilerCode(mb, ast.elseBranch);

        // specify the ending position for the else branch
        OVERWRITE_INT(mb, code, elseParamAddr,
                      CURRENT_ADDRESS(mb, code) - (elseParamAddr + 4));
    }

    else {
        // without an else branch, set the jump destination and move on
        OVERWRITE_INT(mb, code, thenParamAddr,
                      CURRENT_ADDRESS(mb, code) - (thenParamAddr + 4));
    }

    return 0;
}

static unsigned int writeInstructionWhileThen(Xvr_ModuleCompiler** mb,
                                              Xvr_AstWhileThen ast) {
    // begin
    unsigned int beginAddr = CURRENT_ADDRESS(mb, code);

    // cond-branch
    writeModuleCompilerCode(mb, ast.condBranch);

    // emit the jump word (opcode, type, condition, padding)
    EMIT_BYTE(mb, code, XVR_OPCODE_JUMP);
    EMIT_BYTE(mb, code, XVR_OP_PARAM_JUMP_RELATIVE);
    EMIT_BYTE(mb, code, XVR_OP_PARAM_JUMP_IF_FALSE);
    EMIT_BYTE(mb, code, 0);

    unsigned int paramAddr =
        SKIP_INT(mb, code);  // parameter to be written later

    // emit then-branch
    writeModuleCompilerCode(mb, ast.thenBranch);

    // jump to begin to repeat the conditional test
    EMIT_BYTE(mb, code, XVR_OPCODE_JUMP);
    EMIT_BYTE(mb, code, XVR_OP_PARAM_JUMP_RELATIVE);
    EMIT_BYTE(mb, code, XVR_OP_PARAM_JUMP_ALWAYS);
    EMIT_BYTE(mb, code, 0);

    EMIT_INT(mb, code,
             beginAddr - (CURRENT_ADDRESS(mb, code) +
                          4));  // this sets a negative value

    // set the exit parameter for the cond
    OVERWRITE_INT(mb, code, paramAddr,
                  CURRENT_ADDRESS(mb, code) - (paramAddr + 4));

    // set the break & continue data
    while ((*mb)->breakEscapes->count > 0) {
        // extract
        unsigned int addr =
            (*mb)->breakEscapes->data[(*mb)->breakEscapes->count - 1].addr;
        unsigned int depth =
            (*mb)->breakEscapes->data[(*mb)->breakEscapes->count - 1].depth;

        unsigned int diff = depth - (*mb)->currentScopeDepth;

        OVERWRITE_INT(
            mb, code, addr,
            CURRENT_ADDRESS(mb, code) -
                (addr +
                 8));  // tell break to come here AFTER reading the instruction
        OVERWRITE_INT(mb, code, addr, diff);

        // tick down
        (*mb)->breakEscapes->count--;
    }

    while ((*mb)->continueEscapes->count > 0) {
        // extract
        unsigned int addr =
            (*mb)
                ->continueEscapes->data[(*mb)->continueEscapes->count - 1]
                .addr;
        unsigned int depth =
            (*mb)
                ->continueEscapes->data[(*mb)->continueEscapes->count - 1]
                .depth;

        unsigned int diff = depth - (*mb)->currentScopeDepth;

        OVERWRITE_INT(
            mb, code, addr,
            beginAddr - (addr + 8));  // tell continue to return to the start
                                      // AFTER reading the instruction
        OVERWRITE_INT(mb, code, addr, diff);

        // tick down
        (*mb)->continueEscapes->count--;
    }

    return 0;
}

static unsigned int writeInstructionBreak(Xvr_ModuleCompiler** mb,
                                          Xvr_AstBreak ast) {
    // unused
    (void)ast;

    // escapes are always relative
    EMIT_BYTE(mb, code, XVR_OPCODE_ESCAPE);
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);

    unsigned int addr = SKIP_INT(mb, code);
    (void)SKIP_INT(mb, code);  // empty space for depth

    // expand the escape array if needed
    if ((*mb)->breakEscapes->capacity <= (*mb)->breakEscapes->count) {
        (*mb)->breakEscapes = Xvr_private_resizeEscapeArray(
            (*mb)->breakEscapes,
            (*mb)->breakEscapes->capacity * XVR_ESCAPE_EXPANSION_RATE);
    }

    // store for later
    (*mb)->breakEscapes->data[(*mb)->breakEscapes->count++] =
        (Xvr_private_EscapeEntry_t){.addr = addr,
                                    .depth = (*mb)->currentScopeDepth};

    return 0;
}

static unsigned int writeInstructionContinue(Xvr_ModuleCompiler** mb,
                                             Xvr_AstContinue ast) {
    // unused
    (void)ast;

    // escapes are always relative
    EMIT_BYTE(mb, code, XVR_OPCODE_ESCAPE);
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);

    unsigned int addr = SKIP_INT(mb, code);
    (void)SKIP_INT(mb, code);  // empty space for depth

    // expand the escape array if needed
    if ((*mb)->continueEscapes->capacity <= (*mb)->continueEscapes->count) {
        (*mb)->continueEscapes = Xvr_private_resizeEscapeArray(
            (*mb)->continueEscapes,
            (*mb)->continueEscapes->capacity * XVR_ESCAPE_EXPANSION_RATE);
    }

    // store for later
    (*mb)->continueEscapes->data[(*mb)->continueEscapes->count++] =
        (Xvr_private_EscapeEntry_t){.addr = addr,
                                    .depth = (*mb)->currentScopeDepth};

    return 0;
}

static unsigned int writeInstructionPrint(Xvr_ModuleCompiler** mb,
                                          Xvr_AstPrint ast) {
    // the thing to print
    writeModuleCompilerCode(mb, ast.child);

    // output the print opcode
    EMIT_BYTE(mb, code, XVR_OPCODE_PRINT);

    // 4-byte alignment
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);

    return 0;
}

static unsigned int writeInstructionVarDeclare(Xvr_ModuleCompiler** mb,
                                               Xvr_AstVarDeclare ast) {
    // if we're dealing with chained assignments, hijack the next assignment
    // with 'chainedAssignment' set to true
    if (checkForChaining(ast.expr)) {
        writeInstructionAssign(mb, ast.expr->varAssign, true);
    } else {
        writeModuleCompilerCode(mb, ast.expr);  // default value
    }

    // delcare with the given name string
    EMIT_BYTE(mb, code, XVR_OPCODE_DECLARE);
    EMIT_BYTE(mb, code, Xvr_getNameStringVarType(ast.name));
    EMIT_BYTE(
        mb, code,
        ast.name->info.length);  // quick optimisation to skip a 'strlen()' call
    EMIT_BYTE(
        mb, code,
        Xvr_getNameStringVarConstant(ast.name) ? 1 : 0);  // check for constness

    emitString(mb, ast.name);

    return 0;
}

static unsigned int writeInstructionAssign(Xvr_ModuleCompiler** mb,
                                           Xvr_AstVarAssign ast,
                                           bool chainedAssignment) {
    unsigned int result = 0;

    // target is a name string
    if (ast.target->type == XVR_AST_VALUE &&
        XVR_VALUE_IS_STRING(ast.target->value.value) &&
        XVR_VALUE_AS_STRING(ast.target->value.value)->info.type ==
            XVR_STRING_NAME) {
        // name string
        Xvr_String* target = XVR_VALUE_AS_STRING(ast.target->value.value);

        // emit the name string
        EMIT_BYTE(mb, code, XVR_OPCODE_READ);
        EMIT_BYTE(mb, code, XVR_VALUE_STRING);
        EMIT_BYTE(mb, code, XVR_STRING_NAME);
        EMIT_BYTE(mb, code, target->info.length);  // store the length (max 255)

        emitString(mb, target);
    }

    // target is an indexing of some compound value
    else if (ast.target->type == XVR_AST_AGGREGATE &&
             ast.target->aggregate.flag == XVR_AST_FLAG_INDEX) {
        writeModuleCompilerCode(
            mb, ast.target->aggregate.left);  // any deeper indexing will just
                                              // work, using reference values
        writeModuleCompilerCode(mb, ast.target->aggregate.right);  // key

        // if we're dealing with chained assignments, hijack the next assignment
        // with 'chainedAssignment' set to true
        if (checkForChaining(ast.expr)) {
            result += writeInstructionAssign(mb, ast.expr->varAssign, true);
        } else {
            result += writeModuleCompilerCode(mb, ast.expr);  // default value
        }

        EMIT_BYTE(mb, code, XVR_OPCODE_ASSIGN_COMPOUND);  // uses the top three
                                                          // values on the stack
        EMIT_BYTE(mb, code, chainedAssignment);
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        return result + (chainedAssignment ? 1 : 0);
    }

    else {
        // unknown target
        fprintf(stderr, XVR_CC_ERROR
                "COMPILER ERROR: Invalid AST type found: Malformed assignment "
                "target\n" XVR_CC_RESET);
        (*mb)->panic = true;
        return 0;
    }

    // determine RHS, include duplication if needed
    if (ast.flag == XVR_AST_FLAG_ASSIGN) {
        // if we're dealing with chained assignments, hijack the next assignment
        // with 'chainedAssignment' set to true
        if (checkForChaining(ast.expr)) {
            result += writeInstructionAssign(mb, ast.expr->varAssign, true);
        } else {
            result += writeModuleCompilerCode(mb, ast.expr);  // default value
        }

        EMIT_BYTE(mb, code, XVR_OPCODE_ASSIGN);
        EMIT_BYTE(mb, code, chainedAssignment);
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);
    } else if (ast.flag == XVR_AST_FLAG_ADD_ASSIGN) {
        EMIT_BYTE(mb, code, XVR_OPCODE_DUPLICATE);
        EMIT_BYTE(mb, code, XVR_OPCODE_ACCESS);  // squeezed
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        // if we're dealing with chained assignments, hijack the next assignment
        // with 'chainedAssignment' set to true
        if (checkForChaining(ast.expr)) {
            result += writeInstructionAssign(mb, ast.expr->varAssign, true);
        } else {
            result += writeModuleCompilerCode(mb, ast.expr);  // default value
        }

        EMIT_BYTE(mb, code, XVR_OPCODE_ADD);
        EMIT_BYTE(mb, code, XVR_OPCODE_ASSIGN);  // squeezed
        EMIT_BYTE(mb, code, chainedAssignment);
        EMIT_BYTE(mb, code, 0);
    } else if (ast.flag == XVR_AST_FLAG_SUBTRACT_ASSIGN) {
        EMIT_BYTE(mb, code, XVR_OPCODE_DUPLICATE);
        EMIT_BYTE(mb, code, XVR_OPCODE_ACCESS);  // squeezed
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        // if we're dealing with chained assignments, hijack the next assignment
        // with 'chainedAssignment' set to true
        if (checkForChaining(ast.expr)) {
            result += writeInstructionAssign(mb, ast.expr->varAssign, true);
        } else {
            result += writeModuleCompilerCode(mb, ast.expr);  // default value
        }

        EMIT_BYTE(mb, code, XVR_OPCODE_SUBTRACT);
        EMIT_BYTE(mb, code, XVR_OPCODE_ASSIGN);  // squeezed
        EMIT_BYTE(mb, code, chainedAssignment);
        EMIT_BYTE(mb, code, 0);
    } else if (ast.flag == XVR_AST_FLAG_MULTIPLY_ASSIGN) {
        EMIT_BYTE(mb, code, XVR_OPCODE_DUPLICATE);
        EMIT_BYTE(mb, code, XVR_OPCODE_ACCESS);  // squeezed
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        // if we're dealing with chained assignments, hijack the next assignment
        // with 'chainedAssignment' set to true
        if (checkForChaining(ast.expr)) {
            result += writeInstructionAssign(mb, ast.expr->varAssign, true);
        } else {
            result += writeModuleCompilerCode(mb, ast.expr);  // default value
        }

        EMIT_BYTE(mb, code, XVR_OPCODE_MULTIPLY);
        EMIT_BYTE(mb, code, XVR_OPCODE_ASSIGN);  // squeezed
        EMIT_BYTE(mb, code, chainedAssignment);
        EMIT_BYTE(mb, code, 0);
    } else if (ast.flag == XVR_AST_FLAG_DIVIDE_ASSIGN) {
        EMIT_BYTE(mb, code, XVR_OPCODE_DUPLICATE);
        EMIT_BYTE(mb, code, XVR_OPCODE_ACCESS);  // squeezed
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        // if we're dealing with chained assignments, hijack the next assignment
        // with 'chainedAssignment' set to true
        if (checkForChaining(ast.expr)) {
            result += writeInstructionAssign(mb, ast.expr->varAssign, true);
        } else {
            result += writeModuleCompilerCode(mb, ast.expr);  // default value
        }

        EMIT_BYTE(mb, code, XVR_OPCODE_DIVIDE);
        EMIT_BYTE(mb, code, XVR_OPCODE_ASSIGN);  // squeezed
        EMIT_BYTE(mb, code, chainedAssignment);
        EMIT_BYTE(mb, code, 0);
    } else if (ast.flag == XVR_AST_FLAG_MODULO_ASSIGN) {
        EMIT_BYTE(mb, code, XVR_OPCODE_DUPLICATE);
        EMIT_BYTE(mb, code, XVR_OPCODE_ACCESS);  // squeezed
        EMIT_BYTE(mb, code, 0);
        EMIT_BYTE(mb, code, 0);

        // if we're dealing with chained assignments, hijack the next assignment
        // with 'chainedAssignment' set to true
        if (checkForChaining(ast.expr)) {
            result += writeInstructionAssign(mb, ast.expr->varAssign, true);
        } else {
            result += writeModuleCompilerCode(mb, ast.expr);  // default value
        }

        EMIT_BYTE(mb, code, XVR_OPCODE_MODULO);
        EMIT_BYTE(mb, code, XVR_OPCODE_ASSIGN);  // squeezed
        EMIT_BYTE(mb, code, chainedAssignment);
        EMIT_BYTE(mb, code, 0);
    }

    else {
        fprintf(stderr, XVR_CC_ERROR
                "ERROR: Invalid AST assign flag found\n" XVR_CC_RESET);
        exit(-1);
    }

    return result + (chainedAssignment ? 1 : 0);
}

static unsigned int writeInstructionAccess(Xvr_ModuleCompiler** mb,
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

    // push the name
    EMIT_BYTE(mb, code, XVR_OPCODE_READ);
    EMIT_BYTE(mb, code, XVR_VALUE_STRING);
    EMIT_BYTE(mb, code, XVR_STRING_NAME);
    EMIT_BYTE(mb, code, name->info.length);  // store the length (max 255)

    emitString(mb, name);

    // convert name to value
    EMIT_BYTE(mb, code, XVR_OPCODE_ACCESS);
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);
    EMIT_BYTE(mb, code, 0);

    return 1;
}

static unsigned int writeInstructionFnDeclare(Xvr_ModuleCompiler** mb,
                                              Xvr_AstFnDeclare ast) {
    (void)mb;
    (void)ast;
    return 0;
}

static unsigned int writeModuleCompilerCode(Xvr_ModuleCompiler** mb,
                                            Xvr_Ast* ast) {
    if (ast == NULL) {
        return 0;
    }

    // if an error occured, just exit
    if (mb == NULL || (*mb) == NULL || (*mb)->panic) {
        return 0;
    }

    // NOTE: 'result' is used to in 'writeInstructionAggregate()'
    unsigned int result = 0;

    // determine how to write each instruction based on the Ast
    switch (ast->type) {
    case XVR_AST_BLOCK:
        if (ast->block.innerScope) {
            EMIT_BYTE(mb, code, XVR_OPCODE_SCOPE_PUSH);
            EMIT_BYTE(mb, code, 0);
            EMIT_BYTE(mb, code, 0);
            EMIT_BYTE(mb, code, 0);

            (*mb)->currentScopeDepth++;
        }

        result += writeModuleCompilerCode(mb, ast->block.child);
        result += writeModuleCompilerCode(mb, ast->block.next);

        if (ast->block.innerScope) {
            EMIT_BYTE(mb, code, XVR_OPCODE_SCOPE_POP);
            EMIT_BYTE(mb, code, 0);
            EMIT_BYTE(mb, code, 0);
            EMIT_BYTE(mb, code, 0);

            (*mb)->currentScopeDepth--;
        }
        break;

    case XVR_AST_VALUE:
        result += writeInstructionValue(mb, ast->value);
        break;

    case XVR_AST_UNARY:
        result += writeInstructionUnary(mb, ast->unary);
        break;

    case XVR_AST_BINARY:
        result += writeInstructionBinary(mb, ast->binary);
        break;

    case XVR_AST_BINARY_SHORT_CIRCUIT:
        result +=
            writeInstructionBinaryShortCircuit(mb, ast->binaryShortCircuit);
        break;

    case XVR_AST_COMPARE:
        result += writeInstructionCompare(mb, ast->compare);
        break;

    case XVR_AST_GROUP:
        result += writeInstructionGroup(mb, ast->group);
        break;

    case XVR_AST_COMPOUND:
        result += writeInstructionCompound(mb, ast->compound);
        break;

    case XVR_AST_AGGREGATE:
        result += writeInstructionAggregate(mb, ast->aggregate);
        break;

    case XVR_AST_ASSERT:
        result += writeInstructionAssert(mb, ast->assert);
        break;

    case XVR_AST_IF_THEN_ELSE:
        result += writeInstructionIfThenElse(mb, ast->ifThenElse);
        break;

    case XVR_AST_WHILE_THEN:
        result += writeInstructionWhileThen(mb, ast->whileThen);
        break;

    case XVR_AST_BREAK:
        result += writeInstructionBreak(mb, ast->breakPoint);
        break;

    case XVR_AST_CONTINUE:
        result += writeInstructionContinue(mb, ast->continuePoint);
        break;

    case XVR_AST_PRINT:
        result += writeInstructionPrint(mb, ast->print);
        break;

    case XVR_AST_VAR_DECLARE:
        result += writeInstructionVarDeclare(mb, ast->varDeclare);
        break;

    case XVR_AST_VAR_ASSIGN:
        result += writeInstructionAssign(mb, ast->varAssign, false);
        break;

    case XVR_AST_VAR_ACCESS:
        result += writeInstructionAccess(mb, ast->varAccess);
        break;

    case XVR_AST_FN_DECLARE:
        result += writeInstructionFnDeclare(mb, ast->fnDeclare);
        break;

    case XVR_AST_PASS:
        // NO-OP
        break;

    case XVR_AST_ERROR:
        fprintf(stderr, XVR_CC_ERROR
                "COMPILER ERROR: Invalid AST type found: Unknown "
                "'error'\n" XVR_CC_RESET);
        (*mb)->panic = true;
        break;

    case XVR_AST_END:
        fprintf(stderr, XVR_CC_ERROR
                "COMPILER ERROR: Invalid AST type found: Unknown "
                "'end'\n" XVR_CC_RESET);
        (*mb)->panic = true;
        break;
    }

    return result;
}

static void* writeModuleCompiler(Xvr_ModuleCompiler* mb, Xvr_Ast* ast) {
    writeModuleCompilerCode(&mb, ast);

    EMIT_BYTE(&mb, code, XVR_OPCODE_RETURN);  // end terminator
    EMIT_BYTE(&mb, code, 0);                  // 4-byte alignment
    EMIT_BYTE(&mb, code, 0);
    EMIT_BYTE(&mb, code, 0);

    if (mb->panic) {
        return NULL;
    }

    unsigned char* buffer = NULL;
    unsigned int capacity = 0, count = 0;
    int codeAddr = 0;
    int jumpsAddr = 0;
    int dataAddr = 0;

    emitInt(&buffer, &capacity, &count, 0);  // total size (overwritten later)
    emitInt(&buffer, &capacity, &count, mb->jumpsCount);  // jumps size
    emitInt(&buffer, &capacity, &count, mb->paramCount);  // param size
    emitInt(&buffer, &capacity, &count, mb->dataCount);   // data size
    emitInt(&buffer, &capacity, &count, mb->subsCount);   // routine size

    if (mb->codeCount > 0) {
        codeAddr = count;
        emitInt(&buffer, &capacity, &count, 0);  // code
    }
    if (mb->jumpsCount > 0) {
        jumpsAddr = count;
        emitInt(&buffer, &capacity, &count, 0);  // jumps
    }
    if (mb->paramCount > 0) {
        emitInt(&buffer, &capacity, &count, 0);  // params
    }
    if (mb->dataCount > 0) {
        dataAddr = count;
        emitInt(&buffer, &capacity, &count, 0);  // data
    }
    if (mb->subsCount > 0) {
        emitInt(&buffer, &capacity, &count, 0);  // subs
    }

    // append various parts to the buffer
    if (mb->codeCount > 0) {
        expand(&buffer, &capacity, &count, mb->codeCount);
        memcpy((buffer + count), mb->code, mb->codeCount);

        *((int*)(buffer + codeAddr)) = count;
        count += mb->codeCount;
    }

    if (mb->jumpsCount > 0) {
        expand(&buffer, &capacity, &count, mb->jumpsCount);
        memcpy((buffer + count), mb->jumps, mb->jumpsCount);

        *((int*)(buffer + jumpsAddr)) = count;
        count += mb->jumpsCount;
    }

    if (mb->dataCount > 0) {
        expand(&buffer, &capacity, &count, mb->dataCount);
        memcpy((buffer + count), mb->data, mb->dataCount);

        *((int*)(buffer + dataAddr)) = count;
        count += mb->dataCount;
    }

    ((int*)buffer)[0] = count;

    return buffer;
}

void* Xvr_compileModule(Xvr_Ast* ast) {
    Xvr_ModuleCompiler compiler;

    compiler.code = NULL;
    compiler.codeCapacity = 0;
    compiler.codeCount = 0;

    compiler.jumps = NULL;
    compiler.jumpsCapacity = 0;
    compiler.jumpsCount = 0;

    compiler.param = NULL;
    compiler.paramCapacity = 0;
    compiler.paramCount = 0;

    compiler.data = NULL;
    compiler.dataCapacity = 0;
    compiler.dataCount = 0;

    compiler.subs = NULL;
    compiler.subsCapacity = 0;
    compiler.subsCount = 0;

    compiler.currentScopeDepth = 0;
    compiler.breakEscapes =
        Xvr_private_resizeEscapeArray(NULL, XVR_ESCAPE_INITIAL_CAPACITY);
    compiler.continueEscapes =
        Xvr_private_resizeEscapeArray(NULL, XVR_ESCAPE_INITIAL_CAPACITY);

    compiler.panic = false;

    void* buffer = writeModuleCompiler(&compiler, ast);

    Xvr_private_resizeEscapeArray(compiler.breakEscapes, 0);
    Xvr_private_resizeEscapeArray(compiler.continueEscapes, 0);

    free(compiler.param);
    free(compiler.code);
    free(compiler.jumps);
    free(compiler.data);
    free(compiler.subs);

    return buffer;
}
