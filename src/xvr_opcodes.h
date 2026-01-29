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
 * @brief virtual machine bytecode instruction set for the runtime
 *
 * bytecode format
 *   - single-byte opcode (0-255) for compact storage
 *   - variable-length operands (1, 2, 4 bytes) for indices / values
 *   - stack-based execution
 *
 * threading:
 *   - thread-safe for read-only access (no shared mutable state)
 *   - per-thread VM instances for execution
 */

#ifndef XVR_OPCODES_H
#define XVR_OPCODES_H

/**
 * @enum Xvr_Opcode
 * @brief virtual machine instruction  opcode
 *
 * @note values are explicit (no auto-increment to preserve binary
 * compatibility, add new opcodes only at end to maintain serialization)
 */

typedef enum Xvr_Opcode {
    XVR_OP_EOF,  // internally by parser / compiler to mark end of instruction
                 // stream

    // do nothing statements
    XVR_OP_PASS,

    // basic statements
    XVR_OP_ASSERT,  // if `condition` is false, raises runtime error, otherwise
                    // continue
    XVR_OP_PRINT,   // converts `value` to string and outputs to stdout / stderr

    // data
    XVR_OP_LITERAL,       // uint8_t index to constant pool (0 - 255), fasters
                          // literal access for common values
    XVR_OP_LITERAL_LONG,  // uint16_t index to constant pool, this slower than
                          // `LITERAL` due to 2-bytes operand
    XVR_OP_LITERAL_RAW,   // uint8_t index into constant pool, internal
                          // operations, FFI - bypass runtime type safety

    // arithmetic operators
    XVR_OP_NEGATE,          // stack [x] -> [-x]
    XVR_OP_ADDITION,        // stack [x, y] -> [x + y]
    XVR_OP_SUBTRACTION,     // stack [x, y] --> [x - y]
    XVR_OP_MULTIPLICATION,  // stack [x, y] -> [x * y]
    XVR_OP_DIVISION,        // stack [x, y] -> [x / y]
    XVR_OP_MODULO,          // stack [x, y] -> [x % y]
    XVR_OP_GROUPING_BEGIN,  // stack [] -> [], precedence constrol in bytecode
                            // generation
    XVR_OP_GROUPING_END,    // stack [] -> [], precedence control in bytecode
                            // generation

    // variable stuff
    XVR_OP_SCOPE_BEGIN,  // stack [] -> [], push new variable frame into scope
                         // stack
    XVR_OP_SCOPE_END,    // stack [] -> [], pops variable frame, frees local
                         // variable

    XVR_OP_TYPE_DECL,  // stack [] -> [], the operand uint8_t index of type name
                       // in constant pool, register type name in scope (no
                       // value assigned)
    XVR_OP_TYPE_DECL_LONG,  // stack [] -> [], the operand uint16_t index of
                            // type name in constant, pool, same as `TYPE_DECL`,
                            // but for > 255 types

    XVR_OP_VAR_DECL,  // stack [intial_value] -> [], the operand uint8_t index
                      // of variable name in constant pool, binds variable name
                      // to current value in scope
    XVR_OP_VAR_DECL_LONG,  // stack [intial_value] -> [], the operand uint16_t
                           // index of variable name in constant pool, same as
                           // `VAR_DECL` but for > 255 variables

    XVR_OP_FN_DECL,  // stack [function_object] -> [], the operand uint8_t index
                     // of function name in constant pool, binds function name
                     // to bytecode object in scope
    XVR_OP_FN_DECL_LONG,  // stack [function_object] -> [], the operand uint16_t
                          // index of function name in constant pool, same as
                          // `FN_DECL` but for > 255 functions

    XVR_OP_VAR_ASSIGN,  // stack [value] -> [], operand uint8_t index of
                        // variable name in constant pool, store `value` in
                        // named variable
    XVR_OP_VAR_ADDITION_ASSIGN,  // [value] -> [], the operand uint8_t index of
                                 // variable name, for example var = var + value
    XVR_OP_VAR_SUBTRACTION_ASSIGN,  // stack [value] -> [], the operand uint8_t
                                    // index of variable name, for example var =
                                    // var - value
    XVR_OP_VAR_MULTIPLICATION_ASSIGN,  // stack [value] -> [], the operand
                                       // uint8_t index of variable name, for
                                       // example var = var * value
    XVR_OP_VAR_DIVISION_ASSIGN,  // stack [value] -> [], the operand uint8_t
                                 // index of variable name, for example var =
                                 // var % value
    XVR_OP_VAR_MODULO_ASSIGN,

    XVR_OP_TYPE_CAST,  // [value] -> [cast_value], the operand uint8_t index of
                       // target type in constant pool, attempts safe type
                       // conversion
    XVR_OP_TYPE_OF,    // [value] -> [type_literal],  the operand are none, push
                       // XVR_LITERAL_TYPE describing value

    XVR_OP_IMPORT,  // stack [] -> [], the operand uint8_t index of module name
                    // in constant pool, load the module and binds its export to
                    // scope
    XVR_OP_EXPORT_removed,  // stack [] -> [], reserved for backward
                            // compatibility

    // for indexing
    XVR_OP_INDEX,  // stack [container, index] -> [element], the operand are
                   // none, pushes value at `container[index]`
    XVR_OP_INDEX_ASSIGN,  // stack [container, index, value] -> [], the operand
                          // are non, sets container[index] = value
    XVR_OP_INDEX_ASSIGN_INTERMEDIATE,  // stack [container, index, value] -> [],
                                       // operand are none, multi-dimensional
                                       // array assignment
    XVR_OP_DOT,  // stack [object, field_name] -> [field_name], the operand are
                 // none, push value of named field from object

    // comparison of values
    XVR_OP_COMPARE_EQUAL,  // stack [x, y] -> [bool_result], operand are none,
                           // push `true` if `x == y`, `false` otherwise
    XVR_OP_COMPARE_NOT_EQUAL,  // atack [x, y] -> [bool_result], operand are
                               // none, push `true` if `x != y`, `false`
                               // otherwise
    XVR_OP_COMPARE_LESS,  // stack [x, y] -> [bool_result], operand are none,
                          // push `true` if `x < y`, `false` otherwise
    XVR_OP_COMPARE_LESS_EQUAL,  // stack [x, y] -> [bool_result], operand are
                                // none, push `true` if `x <= y`, `false`
                                // otherwise
    XVR_OP_COMPARE_GREATER,  // stack [x, y] -> [bool_result], operand are none,
                             // push `true` uf `x > y`, `false` otherwise
    XVR_OP_COMPARE_GREATER_EQUAL,  // stack [x, y] -> [bool_result], operand are
                                   // none, push `true` if `x >= y`, `false`
                                   // otherwise
    XVR_OP_INVERT,  // stack [x] -> [!x], operand are none, push boolean inverse
                    // of `x`

    // logical operators
    XVR_OP_AND,  // stack [x, y] -> [x && y], operand are none, if `x` is false,
                 // return `x` without evaluating `y`
    XVR_OP_OR,  // stack [x, y] -> [x || y], operand are none, if `x` is truthy,
                // return `x` without evaluating `y`

    // jumps, and conditional jumps (absolute)
    XVR_OP_JUMP,  // stack [] -> [], the operand are uint16_t absolute bytecode
                  // offset, sets instruction pointer to target address
    XVR_OP_IF_FALSE_JUMP,  // stack [condition] -> [], operand are uint16_t
                           // absolute bytecode offset, if `condition` is false,
                           // jumps, oetherwise continues
    XVR_OP_FN_CALL,        // stack [function, arg1, arg2, ..., argN] ->
                           // [return_value], operand are `uint8_t` number of
    // arguments, invokes function with N arguments from stack

    XVR_OP_FN_RETURN,  // stack [return_value] -> [return_value] (on caller's
                       // stack), operand are none, exit current function frame,
                       // return value to caller

    // pop the stack at the end of a complex statement
    XVR_OP_POP_STACK,  // [x] -> [], operand are none, operand are none, clean
                       // up stack after complex expression / statements

    // ternary shorthand
    XVR_OP_TERNARY,  // stack [condition, then_val, else_vale] -> [result], the
                     // operand are none, push `then_val` if `condition` truthy,
                     // else `else_val`

    // meta
    XVR_OP_FN_END,  // stack [] -> [], operand are none, function boundary
                    // detection in debug info
    XVR_OP_SECTION_END = 255,  // stack [] -> [], operand are none, section
                               // boundary detection, padding alignment
} Xvr_Opcode;

#endif  // !XVR_OPCODES_H
