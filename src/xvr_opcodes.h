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

#ifndef XVR_OPCODES_H
#define XVR_OPCODES_H

typedef enum Xvr_Opcode {
    XVR_OP_EOF,

    // basic statements
    XVR_OP_ASSERT,
    XVR_OP_PRINT,

    // data
    XVR_OP_LITERAL,
    XVR_OP_LITERAL_LONG,  // for more than 256 literals in a chunk
    XVR_OP_LITERAL_RAW,   // forcibly get the raw value of the literal

    // arithmetic operators
    XVR_OP_NEGATE,
    XVR_OP_ADDITION,
    XVR_OP_SUBTRACTION,
    XVR_OP_MULTIPLICATION,
    XVR_OP_DIVISION,
    XVR_OP_MODULO,
    XVR_OP_GROUPING_BEGIN,
    XVR_OP_GROUPING_END,

    // variable stuff
    XVR_OP_SCOPE_BEGIN,
    XVR_OP_SCOPE_END,

    XVR_OP_TYPE_DECL,       // declare a type to be used (as a literal)
    XVR_OP_TYPE_DECL_LONG,  // declare a type to be used (as a long literal)

    XVR_OP_VAR_DECL,       // declare a variable to be used (as a literal)
    XVR_OP_VAR_DECL_LONG,  // declare a variable to be used (as a long literal)

    XVR_OP_FN_DECL,       // declare a function to be used (as a literal)
    XVR_OP_FN_DECL_LONG,  // declare a function to be used (as a long literal)

    XVR_OP_VAR_ASSIGN,  // assign to a literal
    XVR_OP_VAR_ADDITION_ASSIGN,
    XVR_OP_VAR_SUBTRACTION_ASSIGN,
    XVR_OP_VAR_MULTIPLICATION_ASSIGN,
    XVR_OP_VAR_DIVISION_ASSIGN,
    XVR_OP_VAR_MODULO_ASSIGN,

    XVR_OP_TYPE_CAST,  // temporarily change a type of an atomic value
    XVR_OP_TYPE_OF,    // get the type of a variable

    XVR_OP_IMPORT,
    XVR_OP_EXPORT_removed,

    // for indexing
    XVR_OP_INDEX,
    XVR_OP_INDEX_ASSIGN,
    XVR_OP_INDEX_ASSIGN_INTERMEDIATE,
    XVR_OP_DOT,

    // comparison of values
    XVR_OP_COMPARE_EQUAL,
    XVR_OP_COMPARE_NOT_EQUAL,
    XVR_OP_COMPARE_LESS,
    XVR_OP_COMPARE_LESS_EQUAL,
    XVR_OP_COMPARE_GREATER,
    XVR_OP_COMPARE_GREATER_EQUAL,
    XVR_OP_INVERT,  // for booleans

    // logical operators
    XVR_OP_AND,
    XVR_OP_OR,

    // jumps, and conditional jumps (absolute)
    XVR_OP_JUMP,
    XVR_OP_IF_FALSE_JUMP,
    XVR_OP_FN_CALL,
    XVR_OP_FN_RETURN,

    // pop the stack at the end of a complex statement
    XVR_OP_POP_STACK,

    // ternary shorthand
    XVR_OP_TERNARY,

    // meta
    XVR_OP_FN_END,  // different from SECTION_END
    XVR_OP_SECTION_END = 255,
} Xvr_Opcode;

#endif  // !XVR_OPCODES_H
