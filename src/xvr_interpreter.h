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
 * @brief stack-based virtual machine interpreter for the runtime
 *
 * execution model:
 *   - stack based: operation pop operands, push results
 *   - lexical scoping: variables resolved via `Xvr_scope` chain
 *   - Native integration: C function callable from bytecode
 *   - hook system: custom identifier resolution
 *
 * memory management:
 *   - interpreter own stack, literalCache and current scope
 *   - bytecode is borrowed (owned by caller)
 *   - hooks dictionary is borrowed (owned by caller)
 *
 * @note use for:
 *   - fast interpretation (switch-based dispatch)
 *   - small memory footprint
 *   - debugging (stack traces, error context)
 *   - FFI integration (native function / hooks)
 */

#ifndef XVR_INTERPRETER_H
#define XVR_INTERPRETER_H

#include "xvr_common.h"
#include "xvr_literal.h"
#include "xvr_literal_array.h"
#include "xvr_literal_dictionary.h"
#include "xvr_scope.h"

/**
 * @typedef Xvr_PrintFn
 * @brief callback for output function (print, error, assert)
 *
 * @param[in] message Null-terminated string to output
 *
 * @note default implementation use `printf` / `fprintf(stderr, ...)` but can be
 * overriden for custom loggin
 */
typedef void (*Xvr_PrintFn)(const char*);

/**
 * @struct Xvr_Interpreter
 * @brief virtual machine state - bytecode execution
 *
 * @note size: 80 bytes - designed for stack allocation if needed
 */
typedef struct Xvr_Interpreter {
    const unsigned char* bytecode;  // current bytecode to execute
    int length;                     // total bytecode size in bytes
    int count;                      // current instruction pointer
    int codeStart;  // start offset of current function (for stack traces)

    Xvr_LiteralArray
        literalCache;  // constant pool from compiler (borrowed reference)

    Xvr_Scope* scope;        // current lexical environment (owned)
    Xvr_LiteralArray stack;  // execution stack (owned)

    Xvr_LiteralDictionary* hooks;  // identifier resolution callbacks (borrowed)

    Xvr_PrintFn printOutput;   // output function for print statement
    Xvr_PrintFn assertOutput;  // output function for assertion failure
    Xvr_PrintFn errorOutput;   // output funrcion for runtime errors

    int depth;   // current call stack depth (for recursion limits)
    bool panic;  // true if urecoverable erro ocurred
} Xvr_Interpreter;

/**
 * @brief intializes interpreter with default strings
 *
 * @param[out] interpreter interpreter to intialize (must not be NULL)
 *
 * @note ssafe to call on zeroed memory, calling twice is safe (but overwrites
 * old state) which is idempotent
 */
XVR_API bool Xvr_injectNativeFn(Xvr_Interpreter* interpreter, const char* name,
                                Xvr_NativeFn func);

XVR_API bool Xvr_injectNativeHook(Xvr_Interpreter* interpreter,
                                  const char* name, Xvr_HookFn hook);

XVR_API bool Xvr_callLiteralFn(Xvr_Interpreter* interpreter, Xvr_Literal func,
                               Xvr_LiteralArray* arguments,
                               Xvr_LiteralArray* returns);
XVR_API bool Xvr_callFn(Xvr_Interpreter* interpreter, const char* name,
                        Xvr_LiteralArray* arguments, Xvr_LiteralArray* returns);

XVR_API bool Xvr_parseIdentifierToValue(Xvr_Interpreter* interpreter,
                                        Xvr_Literal* literalPtr);
XVR_API void Xvr_setInterpreterPrint(Xvr_Interpreter* interpreter,
                                     Xvr_PrintFn printOutput);
XVR_API void Xvr_setInterpreterAssert(Xvr_Interpreter* interpreter,
                                      Xvr_PrintFn assertOutput);

XVR_API void Xvr_setInterpreterError(Xvr_Interpreter* interpreter,
                                     Xvr_PrintFn errorOutput);

/**
 * @brief initializes interpreter with default strings
 *
 * @param[out] interpreter interpreter to initializes (must not be NULL)
 *
 * @note safe to call on zeroed memory, which is calling twice is safe (but
 * overwrites old state)
 */
XVR_API void Xvr_initInterpreter(Xvr_Interpreter* interpreter);

/**
 * @brief execute bytecode from start to end
 *
 * @param[in, out] interpreter interpreter state
 * @param[in] bytecode bytecode buffer to execute
 * @param[in] length size of bytecode in bytes
 *
 * @note execute is immediate - no async or cooperative scheduling
 */
XVR_API void Xvr_runInterpreter(Xvr_Interpreter* interpreter,
                                const unsigned char* bytecode, int length);

/**
 * @brief reset interpreter state for new execution run
 * @param[in, out] interpreter interpreter to reset (must not be NULL)
 *
 * @note use this between bytecode runs to clear temporary value. does not free
 * `scope` - global variables persist
 */
XVR_API void Xvr_resetInterpreter(Xvr_Interpreter* interpreter);

/**
 * @brief free all interpreter resource
 *
 * @param[in, out] interpeter interpreter to destroy (must no NULL)
 *
 * @note safe to call even if `interpeter->panic == true`, safe to call multiple
 * times which is idempotent
 */
XVR_API void Xvr_freeInterpreter(Xvr_Interpreter* interpreter);

#endif  // !XVR_INTERPRETER_H
