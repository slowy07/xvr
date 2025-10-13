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

typedef enum Xvr_OpcodeType {
  // variable instructions
  XVR_OPCODE_READ,
  XVR_OPCODE_DECLARE,
  XVR_OPCODE_ASSIGN,
  XVR_OPCODE_ACCESS,

  XVR_OPCODE_DUPLICATE,

  // arithmetic instructions
  XVR_OPCODE_ADD,
  XVR_OPCODE_SUBTRACT,
  XVR_OPCODE_MULTIPLY,
  XVR_OPCODE_DIVIDE,
  XVR_OPCODE_MODULO,

  // comparison instructions
  XVR_OPCODE_COMPARE_EQUAL,
  // XVR_OPCODE_COMPARE_NOT,
  XVR_OPCODE_COMPARE_LESS,
  XVR_OPCODE_COMPARE_LESS_EQUAL,
  XVR_OPCODE_COMPARE_GREATER,
  XVR_OPCODE_COMPARE_GREATER_EQUAL,

  // logical instructions
  XVR_OPCODE_AND,
  XVR_OPCODE_OR,
  XVR_OPCODE_TRUTHY,
  XVR_OPCODE_NEGATE,

  // control instructions
  XVR_OPCODE_RETURN,
  XVR_OPCODE_JUMP,

  XVR_OPCODE_SCOPE_PUSH,
  XVR_OPCODE_SCOPE_POP,

  XVR_OPCODE_ASSERT,
  XVR_OPCODE_PRINT,
  XVR_OPCODE_CONCAT,
  XVR_OPCODE_INDEX,

  // meta instructions
  XVR_OPCODE_PASS,
  XVR_OPCODE_ERROR,
  XVR_OPCODE_EOF = 255,
} Xvr_OpcodeType;

typedef enum Xvr_OpParamJumpType {
  XVR_OP_PARAM_JUMP_ABSOLUTE = 0,
  XVR_OP_PARAM_JUMP_RELATIVE = 1,
} Xvr_OpJumpType;

typedef enum Xvr_OpParamJumpConditional {
  XVR_OP_PARAM_JUMP_ALWAYS = 0,
  XVR_OP_PARAM_JUMP_IF_TRUE = 1,
  XVR_OP_PARAM_JUMP_IF_FALSE = 2,
} Xvr_OpParamJumpConditional;

#endif // !XVR_OPCODES_H
