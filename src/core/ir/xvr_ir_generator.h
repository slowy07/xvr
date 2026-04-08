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
 * @brief AST to IR translator
 *
 * Converts XVR AST nodes to the internal IR representation
 *
 * memory management:
 *   - all structures are heap-allocated
 *   - Xvr_IRGeneratorDestroy frees all contained objects
 *   - caller is responsible for managing input AST node lifetime
 *
 * threading:
 *   - not thread-safe, external synchronization required
 *
 */

#ifndef XVR_IR_GENERATOR_H
#define XVR_IR_GENERATOR_H

#include "core/ast/xvr_ast_node.h"
#include "core/ir/xvr_ir.h"
#include "ports/output/diagnostics_port.h"

typedef struct Xvr_IRGenerator Xvr_IRGenerator;

Xvr_IRGenerator* Xvr_IRGeneratorCreate(Xvr_DiagnosticsPort* diagnostics);
void Xvr_IRGeneratorDestroy(Xvr_IRGenerator* gen);

Xvr_IRModule* Xvr_IRGeneratorTranslate(Xvr_IRGenerator* gen,
                                       Xvr_ASTNode** ast_nodes, int node_count);

bool Xvr_IRGeneratorHasError(const Xvr_IRGenerator* gen);
const char* Xvr_IRGeneratorGetError(const Xvr_IRGenerator* gen);

#endif
