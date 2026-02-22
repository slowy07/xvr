#include "xvr_compiler.h"

#include <stdio.h>
#include <string.h>

#include "xvr_ast_node.h"
#include "xvr_common.h"
#include "xvr_console_colors.h"
#include "xvr_literal.h"
#include "xvr_literal_array.h"
#include "xvr_literal_dictionary.h"
#include "xvr_memory.h"
#include "xvr_opcodes.h"

void Xvr_initCompiler(Xvr_Compiler* compiler) {
    Xvr_initLiteralArray(&compiler->literalCache);
    compiler->bytecode = NULL;
    compiler->capacity = 0;
    compiler->count = 0;
    compiler->panic = false;
}

// separated out, so it can be recursive
static int writeLiteralTypeToCache(Xvr_LiteralArray* literalCache,
                                   Xvr_Literal literal) {
    bool shouldFree = false;

    // if it's a compound type, recurse and store the results
    if (XVR_AS_TYPE(literal).typeOf == XVR_LITERAL_ARRAY ||
        XVR_AS_TYPE(literal).typeOf == XVR_LITERAL_DICTIONARY) {
        // I don't like storing types in an array, but it's the easiest and most
        // straight forward method
        Xvr_LiteralArray* store = XVR_ALLOCATE(Xvr_LiteralArray, 1);
        Xvr_initLiteralArray(store);

        // store the base literal in the store
        Xvr_pushLiteralArray(store, literal);

        for (int i = 0; i < XVR_AS_TYPE(literal).count; i++) {
            // write the values to the cache, and the indexes to the store
            int subIndex = writeLiteralTypeToCache(
                literalCache,
                ((Xvr_Literal*)(XVR_AS_TYPE(literal).subtypes))[i]);

            Xvr_Literal lit = XVR_TO_INTEGER_LITERAL(subIndex);
            Xvr_pushLiteralArray(store, lit);
            Xvr_freeLiteral(lit);
        }

        // push the store to the cache, tweaking the type
        shouldFree = true;
        literal = XVR_TO_ARRAY_LITERAL(store);
        literal.type =
            XVR_LITERAL_TYPE_INTERMEDIATE;  // NOTE: tweaking the type usually
                                            // isn't a good idea
    }

    // optimisation: check if exactly this literal array exists
    int index = Xvr_findLiteralIndex(literalCache, literal);
    if (index < 0) {
        index = Xvr_pushLiteralArray(literalCache, literal);
    }

    if (shouldFree) {
        Xvr_freeLiteral(literal);
    }
    return index;
}

static int writeNodeCompoundToCache(Xvr_Compiler* compiler, Xvr_ASTNode* node) {
    int index = -1;

    // for both, stored as an array
    Xvr_LiteralArray* store = XVR_ALLOCATE(Xvr_LiteralArray, 1);
    Xvr_initLiteralArray(store);

    // emit an array or a dictionary definition
    if (node->compound.literalType == XVR_LITERAL_DICTIONARY) {
        // ensure each literal key and value are in the cache, individually
        for (int i = 0; i < node->compound.count; i++) {
            // keys
            switch (node->compound.nodes[i].pair.left->type) {
            case XVR_AST_NODE_LITERAL: {
                // keys are literals
                int key = Xvr_findLiteralIndex(
                    &compiler->literalCache,
                    node->compound.nodes[i].pair.left->atomic.literal);
                if (key < 0) {
                    key = Xvr_pushLiteralArray(
                        &compiler->literalCache,
                        node->compound.nodes[i].pair.left->atomic.literal);
                }

                Xvr_Literal literal = XVR_TO_INTEGER_LITERAL(key);
                Xvr_pushLiteralArray(store, literal);
                Xvr_freeLiteral(literal);
            } break;

            case XVR_AST_NODE_COMPOUND: {
                int key = writeNodeCompoundToCache(
                    compiler, node->compound.nodes[i].pair.left);

                Xvr_Literal literal = XVR_TO_INTEGER_LITERAL(key);
                Xvr_pushLiteralArray(store, literal);
                Xvr_freeLiteral(literal);
            } break;

            default:
                fprintf(stderr, XVR_CC_ERROR
                        "[internal] Unrecognized key node type in "
                        "writeNodeCompoundToCache()\n" XVR_CC_RESET);
                return -1;
            }

            // values
            switch (node->compound.nodes[i].pair.right->type) {
            case XVR_AST_NODE_LITERAL: {
                // values are literals
                int val = Xvr_findLiteralIndex(
                    &compiler->literalCache,
                    node->compound.nodes[i].pair.right->atomic.literal);
                if (val < 0) {
                    val = Xvr_pushLiteralArray(
                        &compiler->literalCache,
                        node->compound.nodes[i].pair.right->atomic.literal);
                }

                Xvr_Literal literal = XVR_TO_INTEGER_LITERAL(val);
                Xvr_pushLiteralArray(store, literal);
                Xvr_freeLiteral(literal);
            } break;

            case XVR_AST_NODE_COMPOUND: {
                int val = writeNodeCompoundToCache(
                    compiler, node->compound.nodes[i].pair.right);

                Xvr_Literal literal = XVR_TO_INTEGER_LITERAL(val);
                Xvr_pushLiteralArray(store, literal);
                Xvr_freeLiteral(literal);
            } break;

            default:
                fprintf(stderr, XVR_CC_ERROR
                        "[internal] Unrecognized value node type in "
                        "writeNodeCompoundToCache()\n" XVR_CC_RESET);
                return -1;
            }
        }

        // push the store to the cache, with instructions about how pack it
        Xvr_Literal literal = XVR_TO_DICTIONARY_LITERAL(store);
        literal.type =
            XVR_LITERAL_DICTIONARY_INTERMEDIATE;  // god damn it - nested in a
                                                  // dictionary
        index = Xvr_pushLiteralArray(&compiler->literalCache, literal);
        Xvr_freeLiteral(literal);
    } else if (node->compound.literalType == XVR_LITERAL_ARRAY) {
        // ensure each literal value is in the cache, individually
        for (int i = 0; i < node->compound.count; i++) {
            switch (node->compound.nodes[i].type) {
            case XVR_AST_NODE_LITERAL: {
                // values
                int val = Xvr_findLiteralIndex(
                    &compiler->literalCache,
                    node->compound.nodes[i].atomic.literal);
                if (val < 0) {
                    val = Xvr_pushLiteralArray(
                        &compiler->literalCache,
                        node->compound.nodes[i].atomic.literal);
                }

                Xvr_Literal literal = XVR_TO_INTEGER_LITERAL(val);
                Xvr_pushLiteralArray(store, literal);
                Xvr_freeLiteral(literal);
            } break;

            case XVR_AST_NODE_COMPOUND: {
                int val = writeNodeCompoundToCache(compiler,
                                                   &node->compound.nodes[i]);

                Xvr_Literal literal = XVR_TO_INTEGER_LITERAL(val);
                index = Xvr_pushLiteralArray(store, literal);
                Xvr_freeLiteral(literal);
            } break;

            default:
                fprintf(stderr, XVR_CC_ERROR
                        "[internal] Unrecognized node type in "
                        "writeNodeCompoundToCache()" XVR_CC_RESET);
                return -1;
            }
        }

        // push the store to the cache, with instructions about how pack it
        Xvr_Literal literal = XVR_TO_ARRAY_LITERAL(store);
        literal.type =
            XVR_LITERAL_ARRAY_INTERMEDIATE;  // god damn it - nested in an array
        index = Xvr_pushLiteralArray(&compiler->literalCache, literal);
        Xvr_freeLiteral(literal);
    } else {
        fprintf(stderr, XVR_CC_ERROR
                "[internal] Unrecognized compound type in "
                "writeNodeCompoundToCache()" XVR_CC_RESET);
    }

    return index;
}

static int writeNodeCollectionToCache(Xvr_Compiler* compiler,
                                      Xvr_ASTNode* node) {
    Xvr_LiteralArray* store = XVR_ALLOCATE(Xvr_LiteralArray, 1);
    Xvr_initLiteralArray(store);

    // ensure each literal value is in the cache, individually
    for (int i = 0; i < node->fnCollection.count; i++) {
        switch (node->fnCollection.nodes[i].type) {
        case XVR_AST_NODE_VAR_DECL: {
            // write each piece of the declaration to the cache
            int identifierIndex = Xvr_pushLiteralArray(
                &compiler->literalCache,
                node->fnCollection.nodes[i]
                    .varDecl
                    .identifier);  // store without duplication optimisation
            int typeIndex = writeLiteralTypeToCache(
                &compiler->literalCache,
                node->fnCollection.nodes[i].varDecl.typeLiteral);

            Xvr_Literal identifierLiteral =
                XVR_TO_INTEGER_LITERAL(identifierIndex);
            Xvr_pushLiteralArray(store, identifierLiteral);
            Xvr_freeLiteral(identifierLiteral);

            Xvr_Literal typeLiteral = XVR_TO_INTEGER_LITERAL(typeIndex);
            Xvr_pushLiteralArray(store, typeLiteral);
            Xvr_freeLiteral(typeLiteral);
        } break;

        case XVR_AST_NODE_LITERAL: {
            // write each piece of the declaration to the cache
            int typeIndex = writeLiteralTypeToCache(
                &compiler->literalCache,
                node->fnCollection.nodes[i].atomic.literal);

            Xvr_Literal typeLiteral = XVR_TO_INTEGER_LITERAL(typeIndex);
            Xvr_pushLiteralArray(store, typeLiteral);
            Xvr_freeLiteral(typeLiteral);
        } break;

        default:
            fprintf(stderr, XVR_CC_ERROR
                    "[internal] Unrecognized node type in "
                    "writeNodeCollectionToCache()\n" XVR_CC_RESET);
            return -1;
        }
    }

    // store the store
    Xvr_Literal literal = XVR_TO_ARRAY_LITERAL(store);
    int storeIndex = Xvr_pushLiteralArray(&compiler->literalCache, literal);
    Xvr_freeLiteral(literal);

    return storeIndex;
}

static int writeLiteralToCompiler(Xvr_Compiler* compiler, Xvr_Literal literal) {
    // get the index
    int index = Xvr_findLiteralIndex(&compiler->literalCache, literal);

    if (index < 0) {
        if (XVR_IS_TYPE(literal)) {
            // check for the type literal as value
            index = writeLiteralTypeToCache(&compiler->literalCache, literal);
        } else {
            index = Xvr_pushLiteralArray(&compiler->literalCache, literal);
        }
    }

    // push the literal to the bytecode
    if (index >= 256) {
        // push a "long" index
        compiler->bytecode[compiler->count++] = XVR_OP_LITERAL_LONG;  // 1 byte
        memcpy(compiler->bytecode + compiler->count, &index,
               sizeof(unsigned short));  // 2 bytes

        compiler->count += sizeof(unsigned short);
    } else {
        // push the index
        compiler->bytecode[compiler->count++] = XVR_OP_LITERAL;        // 1 byte
        compiler->bytecode[compiler->count++] = (unsigned char)index;  // 1 byte
    }

    return index;
}

// NOTE: jumpOfsets are included, because function arg and return indexes are
// embedded in the code body i.e. need to include their sizes in the jump NOTE:
// rootNode should NOT include groupings and blocks
static Xvr_Opcode Xvr_writeCompilerWithJumps(
    Xvr_Compiler* compiler, Xvr_ASTNode* node, void* breakAddressesPtr,
    void* continueAddressesPtr, int jumpOffsets, Xvr_ASTNode* rootNode) {
    // grow if the bytecode space is too small
    if (compiler->count + 32 > compiler->capacity) {
        int oldCapacity = compiler->capacity;

        compiler->capacity = XVR_GROW_CAPACITY_FAST(oldCapacity);
        compiler->bytecode = XVR_GROW_ARRAY(unsigned char, compiler->bytecode,
                                            oldCapacity, compiler->capacity);
    }

    // determine node type
    switch (node->type) {
    case XVR_AST_NODE_ERROR: {
        fprintf(stderr, XVR_CC_ERROR
                "[internal] XVR_AST_NODEERROR encountered in "
                "Xvr_writeCompilerWithJumps()\n" XVR_CC_RESET);
        compiler->bytecode[compiler->count++] = XVR_OP_EOF;  // 1 byte
    } break;

    case XVR_AST_NODE_LITERAL: {
        writeLiteralToCompiler(compiler, node->atomic.literal);
    } break;

    case XVR_AST_NODE_UNARY: {
        // pass to the child node, then embed the unary command (print, negate,
        // etc.)
        Xvr_Opcode override = Xvr_writeCompilerWithJumps(
            compiler, node->unary.child, breakAddressesPtr,
            continueAddressesPtr, jumpOffsets, rootNode);

        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        compiler->bytecode[compiler->count++] =
            (unsigned char)node->unary.opcode;  // 1 byte
    } break;

    // all infixes come here
    case XVR_AST_NODE_BINARY: {
        if (node->binary.opcode == XVR_OP_PRINTF) {
            Xvr_ASTNode* compound = node->binary.left;
            int totalCount = compound->compound.count;
            int argCount = totalCount - 1;

            for (int i = totalCount - 1; i >= 0; i--) {
                Xvr_writeCompilerWithJumps(
                    compiler, &compound->compound.nodes[i], breakAddressesPtr,
                    continueAddressesPtr, jumpOffsets, rootNode);
            }

            compiler->bytecode[compiler->count++] =
                (unsigned char)XVR_OP_PRINTF;
            compiler->bytecode[compiler->count++] = (unsigned char)argCount;

            return XVR_OP_EOF;
        }

        // pass to the child nodes, then embed the binary command (math, etc.)
        Xvr_Opcode override = Xvr_writeCompilerWithJumps(
            compiler, node->binary.left, breakAddressesPtr,
            continueAddressesPtr, jumpOffsets, rootNode);

        // special case for when indexing and assigning
        if (override != XVR_OP_EOF &&
            node->binary.opcode >= XVR_OP_VAR_ASSIGN &&
            node->binary.opcode <= XVR_OP_VAR_MODULO_ASSIGN) {
            Xvr_writeCompilerWithJumps(compiler, node->binary.right,
                                       breakAddressesPtr, continueAddressesPtr,
                                       jumpOffsets, rootNode);

            if (node->binary.left->type == XVR_AST_NODE_BINARY &&
                node->binary.right->type == XVR_AST_NODE_BINARY &&
                node->binary.left->binary.opcode == XVR_OP_INDEX &&
                node->binary.right->binary.opcode == XVR_OP_INDEX) {
                compiler->bytecode[compiler->count++] =
                    (unsigned char)XVR_OP_INDEX;
            }

            compiler->bytecode[compiler->count++] = (unsigned char)
                XVR_OP_INDEX_ASSIGN;  // 1 byte WARNING: enum trickery
            compiler->bytecode[compiler->count++] =
                (unsigned char)node->binary.opcode;  // 1 byte
            return XVR_OP_EOF;
        }

        // compensate for... yikes
        if (override != XVR_OP_EOF) {
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        // return this if...
        Xvr_Opcode ret = Xvr_writeCompilerWithJumps(
            compiler, node->binary.right, breakAddressesPtr,
            continueAddressesPtr, jumpOffsets, rootNode);

        if (node->binary.opcode == XVR_OP_INDEX &&
            rootNode->type == XVR_AST_NODE_BINARY &&
            (rootNode->binary.opcode >= XVR_OP_VAR_ASSIGN &&
             rootNode->binary.opcode <= XVR_OP_VAR_MODULO_ASSIGN) &&
            rootNode->binary.right != node) {  // range-based check for
                                               // assignment type
            return XVR_OP_INDEX_ASSIGN_INTERMEDIATE;
        }

        // loopy logic - if opcode == index or dot
        if (node->binary.opcode == XVR_OP_INDEX ||
            node->binary.opcode == XVR_OP_DOT) {
            return node->binary.opcode;
        }

        if (ret != XVR_OP_EOF &&
            (node->binary.opcode == XVR_OP_VAR_ASSIGN ||
             node->binary.opcode == XVR_OP_AND ||
             node->binary.opcode == XVR_OP_OR ||
             (node->binary.opcode >= XVR_OP_COMPARE_EQUAL &&
              node->binary.opcode <= XVR_OP_INVERT))) {
            compiler->bytecode[compiler->count++] =
                (unsigned char)ret;  // 1 byte
            ret = XVR_OP_EOF;        // untangle in this case
        }

        compiler->bytecode[compiler->count++] =
            (unsigned char)node->binary.opcode;  // 1 byte

        return ret;
    } break;

    case XVR_AST_NODE_TERNARY: {
        // TODO

        // process the condition
        Xvr_Opcode override = Xvr_writeCompilerWithJumps(
            compiler, node->ternary.condition, breakAddressesPtr,
            continueAddressesPtr, jumpOffsets, rootNode);
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        // cache the point to insert the jump distance at
        compiler->bytecode[compiler->count++] = XVR_OP_IF_FALSE_JUMP;  // 1 byte
        int jumpToElse = compiler->count;
        compiler->count += sizeof(unsigned short);  // 2 bytes

        // write the then path
        override = Xvr_writeCompilerWithJumps(
            compiler, node->ternary.thenPath, breakAddressesPtr,
            continueAddressesPtr, jumpOffsets, rootNode);
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        int jumpToEnd = 0;

        // insert jump to end
        compiler->bytecode[compiler->count++] = XVR_OP_JUMP;  // 1 byte
        jumpToEnd = compiler->count;
        compiler->count += sizeof(unsigned short);  // 2 bytes

        // update the jumpToElse to point here
        unsigned short tmpVal = compiler->count + jumpOffsets;
        memcpy(compiler->bytecode + jumpToElse, &tmpVal,
               sizeof(tmpVal));  // 2 bytes

        // write the else path
        Xvr_Opcode override2 = Xvr_writeCompilerWithJumps(
            compiler, node->ternary.elsePath, breakAddressesPtr,
            continueAddressesPtr, jumpOffsets, rootNode);
        if (override2 != XVR_OP_EOF) {  // compensate for indexing & dot
                                        // notation being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char)override2;  // 1 byte
        }

        // update the jumpToEnd to point here
        tmpVal = compiler->count + jumpOffsets;
        memcpy(compiler->bytecode + jumpToEnd, &tmpVal,
               sizeof(tmpVal));  // 2 bytes
    } break;

    case XVR_AST_NODE_GROUPING: {
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_GROUPING_BEGIN;  // 1 byte
        Xvr_Opcode override = Xvr_writeCompilerWithJumps(
            compiler, node->grouping.child, breakAddressesPtr,
            continueAddressesPtr, jumpOffsets, node->grouping.child);
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_GROUPING_END;  // 1 byte
    } break;

    case XVR_AST_NODE_BLOCK: {
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_SCOPE_BEGIN;  // 1 byte

        for (int i = 0; i < node->block.count; i++) {
            Xvr_Opcode override = Xvr_writeCompilerWithJumps(
                compiler, &(node->block.nodes[i]), breakAddressesPtr,
                continueAddressesPtr, jumpOffsets, &(node->block.nodes[i]));
            if (override != XVR_OP_EOF) {  // compensate for indexing & dot
                                           // notation being screwy
                compiler->bytecode[compiler->count++] =
                    (unsigned char) override;  // 1 byte
            }
        }

        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_SCOPE_END;  // 1 byte
    } break;

    case XVR_AST_NODE_COMPOUND: {
        int index = writeNodeCompoundToCache(compiler, node);

        if (index < 0) {
            compiler->panic = true;
            return XVR_OP_EOF;
        }

        // push the node opcode to the bytecode
        if (index >= 256) {
            // push a "long" index
            compiler->bytecode[compiler->count++] =
                XVR_OP_LITERAL_LONG;  // 1 byte
            memcpy(compiler->bytecode + compiler->count, &index,
                   sizeof(unsigned short));

            compiler->count += sizeof(unsigned short);
        } else {
            // push the index
            compiler->bytecode[compiler->count++] = XVR_OP_LITERAL;  // 1 byte
            compiler->bytecode[compiler->count++] =
                (unsigned char)index;  // 1 byte
        }
    } break;

    case XVR_AST_NODE_PAIR:
        fprintf(stderr, XVR_CC_ERROR
                "[internal] XVR_AST_NODEPAIR encountered in "
                "Xvr_writeCompilerWithJumps()\n" XVR_CC_RESET);
        compiler->bytecode[compiler->count++] = XVR_OP_EOF;  // 1 byte
        break;

    case XVR_AST_NODE_VAR_DECL: {
        // first, embed the expression (leaves it on the stack)
        Xvr_Opcode override = Xvr_writeCompilerWithJumps(
            compiler, node->varDecl.expression, breakAddressesPtr,
            continueAddressesPtr, jumpOffsets, rootNode);
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        // write each piece of the declaration to the bytecode
        int identifierIndex = Xvr_findLiteralIndex(&compiler->literalCache,
                                                   node->varDecl.identifier);
        if (identifierIndex < 0) {
            identifierIndex = Xvr_pushLiteralArray(&compiler->literalCache,
                                                   node->varDecl.identifier);
        }

        int typeIndex = writeLiteralTypeToCache(&compiler->literalCache,
                                                node->varDecl.typeLiteral);

        // embed the info into the bytecode
        if (identifierIndex >= 256 || typeIndex >= 256) {
            // push a "long" declaration
            compiler->bytecode[compiler->count++] =
                XVR_OP_VAR_DECL_LONG;  // 1 byte

            *((unsigned short*)(compiler->bytecode + compiler->count)) =
                (unsigned short)identifierIndex;  // 2 bytes
            compiler->count += sizeof(unsigned short);

            *((unsigned short*)(compiler->bytecode + compiler->count)) =
                (unsigned short)typeIndex;  // 2 bytes
            compiler->count += sizeof(unsigned short);
        } else {
            // push a declaration
            compiler->bytecode[compiler->count++] = XVR_OP_VAR_DECL;  // 1 byte
            compiler->bytecode[compiler->count++] =
                (unsigned char)identifierIndex;  // 1 byte
            compiler->bytecode[compiler->count++] =
                (unsigned char)typeIndex;  // 1 byte
        }
    } break;

    case XVR_AST_NODE_FN_DECL: {
        // run a compiler over the function
        Xvr_Compiler* fnCompiler = XVR_ALLOCATE(Xvr_Compiler, 1);
        Xvr_initCompiler(fnCompiler);
        Xvr_writeCompiler(
            fnCompiler, node->fnDecl.arguments);  // can be empty, but not NULL
        Xvr_writeCompiler(fnCompiler,
                          node->fnDecl.returns);  // can be empty, but not NULL
        Xvr_Opcode override = Xvr_writeCompilerWithJumps(
            fnCompiler, node->fnDecl.block, NULL, NULL, -4,
            rootNode);                 // can be empty, but not NULL
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        if (fnCompiler->panic) {
            compiler->panic = true;
        }

        // create the function in the literal cache (by storing the compiler
        // object)
        Xvr_Literal fnLiteral = XVR_TO_FUNCTION_LITERAL(fnCompiler, 0);
        fnLiteral.type =
            XVR_LITERAL_FUNCTION_INTERMEDIATE;  // NOTE: changing type

        // push the name
        int identifierIndex = Xvr_findLiteralIndex(&compiler->literalCache,
                                                   node->fnDecl.identifier);
        if (identifierIndex < 0) {
            identifierIndex = Xvr_pushLiteralArray(&compiler->literalCache,
                                                   node->fnDecl.identifier);
        }

        // push to function (functions are never equal)
        int fnIndex = Xvr_pushLiteralArray(&compiler->literalCache, fnLiteral);

        // embed the info into the bytecode
        if (identifierIndex >= 256 || fnIndex >= 256) {
            // push a "long" declaration
            compiler->bytecode[compiler->count++] =
                XVR_OP_FN_DECL_LONG;  // 1 byte

            *((unsigned short*)(compiler->bytecode + compiler->count)) =
                (unsigned short)identifierIndex;  // 2 bytes
            compiler->count += sizeof(unsigned short);

            *((unsigned short*)(compiler->bytecode + compiler->count)) =
                (unsigned short)fnIndex;  // 2 bytes
            compiler->count += sizeof(unsigned short);
        } else {
            // push a declaration
            compiler->bytecode[compiler->count++] = XVR_OP_FN_DECL;  // 1 byte
            compiler->bytecode[compiler->count++] =
                (unsigned char)identifierIndex;  // 1 byte
            compiler->bytecode[compiler->count++] =
                (unsigned char)fnIndex;  // 1 byte
        }
    } break;

    case XVR_AST_NODE_FN_COLLECTION: {
        // embed these in the bytecode...
        unsigned short index =
            (unsigned short)writeNodeCollectionToCache(compiler, node);

        if (index == (unsigned short)-1) {
            compiler->panic = true;
            return XVR_OP_EOF;
        }

        memcpy(compiler->bytecode + compiler->count, &index, sizeof(index));
        compiler->count += sizeof(unsigned short);
    } break;

    case XVR_AST_NODE_FN_CALL: {
        for (int i = 0; i < node->fnCall.arguments->fnCollection.count; i++) {
            if (node->fnCall.arguments->fnCollection.nodes[i].type !=
                XVR_AST_NODE_LITERAL) {
                Xvr_Opcode override = Xvr_writeCompilerWithJumps(
                    compiler, &node->fnCall.arguments->fnCollection.nodes[i],
                    breakAddressesPtr, continueAddressesPtr, jumpOffsets,
                    rootNode);
                if (override != XVR_OP_EOF) {  // compensate for indexing & dot
                                               // notation being screwy
                    compiler->bytecode[compiler->count++] =
                        (unsigned char) override;  // 1 byte
                }
                continue;
            }

            // write each argument to the bytecode
            int argumentsIndex = Xvr_findLiteralIndex(
                &compiler->literalCache,
                node->fnCall.arguments->fnCollection.nodes[i].atomic.literal);
            if (argumentsIndex < 0) {
                argumentsIndex = Xvr_pushLiteralArray(
                    &compiler->literalCache,
                    node->fnCall.arguments->fnCollection.nodes[i]
                        .atomic.literal);
            }

            // push the node opcode to the bytecode
            if (argumentsIndex >= 256) {
                // push a "long" index
                compiler->bytecode[compiler->count++] =
                    XVR_OP_LITERAL_LONG;  // 1 byte

                *((unsigned short*)(compiler->bytecode + compiler->count)) =
                    (unsigned short)argumentsIndex;  // 2 bytes
                compiler->count += sizeof(unsigned short);
            } else {
                // push the index
                compiler->bytecode[compiler->count++] =
                    XVR_OP_LITERAL;  // 1 byte
                compiler->bytecode[compiler->count++] =
                    (unsigned char)argumentsIndex;  // 1 byte
            }
        }

        // push the argument COUNT to the top of the stack
        Xvr_Literal argumentsCountLiteral = XVR_TO_INTEGER_LITERAL(
            node->fnCall.argumentCount);  // argumentCount is set elsewhere to
                                          // support dot operator
        int argumentsCountIndex = Xvr_findLiteralIndex(&compiler->literalCache,
                                                       argumentsCountLiteral);
        if (argumentsCountIndex < 0) {
            argumentsCountIndex = Xvr_pushLiteralArray(&compiler->literalCache,
                                                       argumentsCountLiteral);
        }
        Xvr_freeLiteral(argumentsCountLiteral);

        if (argumentsCountIndex >= 256) {
            // push a "long" index
            compiler->bytecode[compiler->count++] =
                XVR_OP_LITERAL_LONG;  // 1 byte

            *((unsigned short*)(compiler->bytecode + compiler->count)) =
                (unsigned short)argumentsCountIndex;  // 2 bytes
            compiler->count += sizeof(unsigned short);
        } else {
            // push the index
            compiler->bytecode[compiler->count++] = XVR_OP_LITERAL;  // 1 byte
            compiler->bytecode[compiler->count++] =
                (unsigned char)argumentsCountIndex;  // 1 byte
        }

        // call the function
        // DO NOT call the collection, this is done in binary
    } break;

    case XVR_AST_NODE_IF: {
        // process the condition
        Xvr_Opcode override = Xvr_writeCompilerWithJumps(
            compiler, node->pathIf.condition, breakAddressesPtr,
            continueAddressesPtr, jumpOffsets, rootNode);
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        // cache the point to insert the jump distance at
        compiler->bytecode[compiler->count++] = XVR_OP_IF_FALSE_JUMP;  // 1 byte
        int jumpToElse = compiler->count;
        compiler->count += sizeof(unsigned short);  // 2 bytes

        // write the then path
        override = Xvr_writeCompilerWithJumps(
            compiler, node->pathIf.thenPath, breakAddressesPtr,
            continueAddressesPtr, jumpOffsets, rootNode);
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        int jumpToEnd = 0;

        if (node->pathIf.elsePath) {
            // insert jump to end
            compiler->bytecode[compiler->count++] = XVR_OP_JUMP;  // 1 byte
            jumpToEnd = compiler->count;
            compiler->count += sizeof(unsigned short);  // 2 bytes
        }

        // update the jumpToElse to point here
        unsigned short tmpVal = compiler->count + jumpOffsets;
        memcpy(compiler->bytecode + jumpToElse, &tmpVal,
               sizeof(tmpVal));  // 2 bytes

        if (node->pathIf.elsePath) {
            // if there's an else path, write it and
            Xvr_Opcode override = Xvr_writeCompilerWithJumps(
                compiler, node->pathIf.elsePath, breakAddressesPtr,
                continueAddressesPtr, jumpOffsets, rootNode);
            if (override != XVR_OP_EOF) {  // compensate for indexing & dot
                                           // notation being screwy
                compiler->bytecode[compiler->count++] =
                    (unsigned char) override;  // 1 byte
            }

            // update the jumpToEnd to point here
            tmpVal = compiler->count + jumpOffsets;
            memcpy(compiler->bytecode + jumpToEnd, &tmpVal,
                   sizeof(tmpVal));  // 2 bytes
        }
    } break;

    case XVR_AST_NODE_WHILE: {
        // for breaks and continues
        Xvr_LiteralArray breakAddresses;
        Xvr_LiteralArray continueAddresses;

        Xvr_initLiteralArray(&breakAddresses);
        Xvr_initLiteralArray(&continueAddresses);

        // cache the jump point
        unsigned short jumpToStart = compiler->count;

        // process the condition
        Xvr_Opcode override = Xvr_writeCompilerWithJumps(
            compiler, node->pathWhile.condition, &breakAddresses,
            &continueAddresses, jumpOffsets, rootNode);
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        // if false, jump to end
        compiler->bytecode[compiler->count++] = XVR_OP_IF_FALSE_JUMP;  // 1 byte
        unsigned short jumpToEnd = compiler->count;
        compiler->count += sizeof(unsigned short);  // 2 bytes

        // write the body
        override = Xvr_writeCompilerWithJumps(
            compiler, node->pathWhile.thenPath, &breakAddresses,
            &continueAddresses, jumpOffsets, rootNode);
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        // jump to condition
        compiler->bytecode[compiler->count++] = XVR_OP_JUMP;  // 1 byte
        unsigned short tmpVal = jumpToStart + jumpOffsets;
        memcpy(compiler->bytecode + compiler->count, &tmpVal, sizeof(tmpVal));
        compiler->count += sizeof(unsigned short);  // 2 bytes

        // jump from condition
        tmpVal = compiler->count + jumpOffsets;
        memcpy(compiler->bytecode + jumpToEnd, &tmpVal, sizeof(tmpVal));

        // set the breaks and continues
        for (int i = 0; i < breakAddresses.count; i++) {
            int point = XVR_AS_INTEGER(breakAddresses.literals[i]);
            tmpVal = compiler->count + jumpOffsets;
            memcpy(compiler->bytecode + point, &tmpVal, sizeof(tmpVal));
        }

        for (int i = 0; i < continueAddresses.count; i++) {
            int point = XVR_AS_INTEGER(continueAddresses.literals[i]);
            tmpVal = jumpToStart + jumpOffsets;
            memcpy(compiler->bytecode + point, &tmpVal, sizeof(tmpVal));
        }

        // clear the stack after use
        compiler->bytecode[compiler->count++] = XVR_OP_POP_STACK;  // 1 byte

        // cleanup
        Xvr_freeLiteralArray(&breakAddresses);
        Xvr_freeLiteralArray(&continueAddresses);
    } break;

    case XVR_AST_NODE_FOR: {
        // for breaks and continues
        Xvr_LiteralArray breakAddresses;
        Xvr_LiteralArray continueAddresses;

        Xvr_initLiteralArray(&breakAddresses);
        Xvr_initLiteralArray(&continueAddresses);

        compiler->bytecode[compiler->count++] = XVR_OP_SCOPE_BEGIN;  // 1 byte

        // initial setup
        Xvr_Opcode override = Xvr_writeCompilerWithJumps(
            compiler, node->pathFor.preClause, &breakAddresses,
            &continueAddresses, jumpOffsets, rootNode);
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        // conditional
        unsigned short jumpToStart = compiler->count;
        override = Xvr_writeCompilerWithJumps(
            compiler, node->pathFor.condition, &breakAddresses,
            &continueAddresses, jumpOffsets, rootNode);
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        // if false jump to end
        compiler->bytecode[compiler->count++] = XVR_OP_IF_FALSE_JUMP;  // 1 byte
        unsigned short jumpToEnd = compiler->count;
        compiler->count += sizeof(unsigned short);  // 2 bytes

        // write the body
        compiler->bytecode[compiler->count++] = XVR_OP_SCOPE_BEGIN;  // 1 byte
        override = Xvr_writeCompilerWithJumps(
            compiler, node->pathFor.thenPath, &breakAddresses,
            &continueAddresses, jumpOffsets, rootNode);
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }
        compiler->bytecode[compiler->count++] = XVR_OP_SCOPE_END;  // 1 byte

        // for-breaks actually jump to the bottom
        int jumpToIncrement = compiler->count;

        // evaluate third clause, restart
        override = Xvr_writeCompilerWithJumps(
            compiler, node->pathFor.postClause, &breakAddresses,
            &continueAddresses, jumpOffsets, rootNode);
        if (override != XVR_OP_EOF) {  // compensate for indexing & dot notation
                                       // being screwy
            compiler->bytecode[compiler->count++] =
                (unsigned char) override;  // 1 byte
        }

        compiler->bytecode[compiler->count++] = XVR_OP_JUMP;  // 1 byte
        unsigned short tmpVal = jumpToStart + jumpOffsets;
        memcpy(compiler->bytecode + compiler->count, &tmpVal, sizeof(tmpVal));
        compiler->count += sizeof(unsigned short);  // 2 bytes

        tmpVal = compiler->count + jumpOffsets;
        memcpy(compiler->bytecode + jumpToEnd, &tmpVal, sizeof(tmpVal));

        compiler->bytecode[compiler->count++] = XVR_OP_SCOPE_END;  // 1 byte

        // set the breaks and continues
        for (int i = 0; i < breakAddresses.count; i++) {
            int point = XVR_AS_INTEGER(breakAddresses.literals[i]);
            tmpVal = compiler->count + jumpOffsets;
            memcpy(compiler->bytecode + point, &tmpVal, sizeof(tmpVal));
        }

        for (int i = 0; i < continueAddresses.count; i++) {
            int point = XVR_AS_INTEGER(continueAddresses.literals[i]);
            tmpVal = jumpToIncrement + jumpOffsets;
            memcpy(compiler->bytecode + point, &tmpVal, sizeof(tmpVal));
        }

        // clear the stack after use
        compiler->bytecode[compiler->count++] = XVR_OP_POP_STACK;  // 1 byte

        // cleanup
        Xvr_freeLiteralArray(&breakAddresses);
        Xvr_freeLiteralArray(&continueAddresses);
    } break;

    case XVR_AST_NODE_BREAK: {
        if (!breakAddressesPtr) {
            fprintf(stderr, XVR_CC_ERROR
                    "XVR_CC_ERROR: Can't place a break statement "
                    "here\n" XVR_CC_RESET);
            break;
        }

        // insert into bytecode
        compiler->bytecode[compiler->count++] = XVR_OP_JUMP;  // 1 byte

        // push to the breakAddresses array
        Xvr_Literal literal = XVR_TO_INTEGER_LITERAL(compiler->count);
        Xvr_pushLiteralArray((Xvr_LiteralArray*)breakAddressesPtr, literal);
        Xvr_freeLiteral(literal);

        compiler->count += sizeof(unsigned short);  // 2 bytes
    } break;

    case XVR_AST_NODE_CONTINUE: {
        if (!continueAddressesPtr) {
            fprintf(stderr, XVR_CC_ERROR
                    "XVR_CC_ERROR: Can't place a continue statement "
                    "here\n" XVR_CC_RESET);
            break;
        }

        // insert into bytecode
        compiler->bytecode[compiler->count++] = XVR_OP_JUMP;  // 1 byte

        // push to the continueAddresses array
        Xvr_Literal literal = XVR_TO_INTEGER_LITERAL(compiler->count);
        Xvr_pushLiteralArray((Xvr_LiteralArray*)continueAddressesPtr, literal);
        Xvr_freeLiteral(literal);

        compiler->count += sizeof(unsigned short);  // 2 bytes
    } break;

    case XVR_AST_NODE_FN_RETURN: {
        // read each returned literal onto the stack, and return the number of
        // values to return
        for (int i = 0; i < node->returns.returns->fnCollection.count; i++) {
            Xvr_Opcode override = Xvr_writeCompilerWithJumps(
                compiler, &node->returns.returns->fnCollection.nodes[i],
                breakAddressesPtr, continueAddressesPtr, jumpOffsets, rootNode);
            if (override != XVR_OP_EOF) {  // compensate for indexing & dot
                                           // notation being screwy
                compiler->bytecode[compiler->count++] =
                    (unsigned char) override;  // 1 byte
            }
        }

        // push the return, with the number of literals
        compiler->bytecode[compiler->count++] = XVR_OP_FN_RETURN;  // 1 byte

        memcpy(compiler->bytecode + compiler->count,
               &node->returns.returns->fnCollection.count,
               sizeof(unsigned short));
        compiler->count += sizeof(unsigned short);
    } break;

    case XVR_AST_NODE_PREFIX_INCREMENT: {
        // push the literal to the stack (twice: add + assign)
        writeLiteralToCompiler(compiler, node->prefixIncrement.identifier);
        writeLiteralToCompiler(compiler, node->prefixIncrement.identifier);

        // push the increment / decrement
        Xvr_Literal increment = XVR_TO_INTEGER_LITERAL(1);
        writeLiteralToCompiler(compiler, increment);

        // push the add opcode
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_ADDITION;  // 1 byte

        // push the assign
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_VAR_ASSIGN;  // 1 byte

        // leave the result on the stack
        writeLiteralToCompiler(compiler, node->prefixIncrement.identifier);
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_LITERAL_RAW;  // 1 byte
    } break;

    case XVR_AST_NODE_PREFIX_DECREMENT: {
        // push the literal to the stack (twice: add + assign)
        writeLiteralToCompiler(compiler, node->prefixDecrement.identifier);
        writeLiteralToCompiler(compiler, node->prefixDecrement.identifier);

        // push the increment / decrement
        Xvr_Literal increment = XVR_TO_INTEGER_LITERAL(1);
        writeLiteralToCompiler(compiler, increment);

        // push the subtract opcode
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_SUBTRACTION;  // 1 byte

        // push the assign
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_VAR_ASSIGN;  // 1 byte

        // leave the result on the stack
        writeLiteralToCompiler(compiler, node->prefixDecrement.identifier);
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_LITERAL_RAW;  // 1 byte
    } break;

    case XVR_AST_NODE_POSTFIX_INCREMENT: {
        // push the identifier's VALUE to the stack
        writeLiteralToCompiler(compiler, node->postfixIncrement.identifier);
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_LITERAL_RAW;  // 1 byte

        // push the identifier (twice: add + assign)
        writeLiteralToCompiler(compiler, node->postfixIncrement.identifier);
        writeLiteralToCompiler(compiler, node->postfixIncrement.identifier);

        // push the increment / decrement
        Xvr_Literal increment = XVR_TO_INTEGER_LITERAL(1);
        writeLiteralToCompiler(compiler, increment);

        // push the add opcode
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_ADDITION;  // 1 byte

        // push the assign
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_VAR_ASSIGN;  // 1 byte
    } break;

    case XVR_AST_NODE_POSTFIX_DECREMENT: {
        // push the identifier's VALUE to the stack
        writeLiteralToCompiler(compiler, node->postfixDecrement.identifier);
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_LITERAL_RAW;  // 1 byte

        // push the identifier (twice: add + assign)
        writeLiteralToCompiler(compiler, node->postfixDecrement.identifier);
        writeLiteralToCompiler(compiler, node->postfixDecrement.identifier);

        // push the increment / decrement
        Xvr_Literal increment = XVR_TO_INTEGER_LITERAL(1);
        writeLiteralToCompiler(compiler, increment);

        // push the subtract opcode
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_SUBTRACTION;  // 1 byte

        // push the assign
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_VAR_ASSIGN;  // 1 byte
    } break;

    case XVR_AST_NODE_IMPORT: {
        // push the identifier, and the alias
        writeLiteralToCompiler(compiler, node->import.identifier);
        writeLiteralToCompiler(compiler, node->import.alias);

        // push the import opcode
        compiler->bytecode[compiler->count++] =
            (unsigned char)XVR_OP_IMPORT;  // 1 byte
    } break;

    case XVR_AST_NODE_INDEX: {
        if (!node->index.first) {
            writeLiteralToCompiler(compiler, XVR_TO_NULL_LITERAL);
        } else {
            Xvr_Opcode override = Xvr_writeCompilerWithJumps(
                compiler, node->index.first, breakAddressesPtr,
                continueAddressesPtr, jumpOffsets, rootNode);
            if (override != XVR_OP_EOF) {  // compensate for indexing & dot
                                           // notation being screwy
                compiler->bytecode[compiler->count++] =
                    (unsigned char) override;  // 1 byte
            }
        }

        // second
        if (!node->index.second) {
            writeLiteralToCompiler(compiler, XVR_TO_NULL_LITERAL);
        } else {
            Xvr_Opcode override = Xvr_writeCompilerWithJumps(
                compiler, node->index.second, breakAddressesPtr,
                continueAddressesPtr, jumpOffsets, rootNode);
            if (override != XVR_OP_EOF) {  // compensate for indexing & dot
                                           // notation being screwy
                compiler->bytecode[compiler->count++] =
                    (unsigned char) override;  // 1 byte
            }
        }

        // third
        if (!node->index.third) {
            writeLiteralToCompiler(compiler, XVR_TO_NULL_LITERAL);
        } else {
            Xvr_Opcode override = Xvr_writeCompilerWithJumps(
                compiler, node->index.third, breakAddressesPtr,
                continueAddressesPtr, jumpOffsets, rootNode);
            if (override != XVR_OP_EOF) {  // compensate for indexing & dot
                                           // notation being screwy
                compiler->bytecode[compiler->count++] =
                    (unsigned char) override;  // 1 byte
            }
        }

        return XVR_OP_INDEX_ASSIGN;
    } break;

    case XVR_AST_NODE_PASS: {
        return XVR_OP_PASS;
    } break;
    }

    return XVR_OP_EOF;
}

void Xvr_writeCompiler(Xvr_Compiler* compiler, Xvr_ASTNode* node) {
    Xvr_Opcode op =
        Xvr_writeCompilerWithJumps(compiler, node, NULL, NULL, 0,
                                   node);  // pass in "node" as the root node

    if (op !=
        XVR_OP_EOF) {  // compensate for indexing & dot notation being screwy
        compiler->bytecode[compiler->count++] = (unsigned char)op;  // 1 byte
    }
}

void Xvr_freeCompiler(Xvr_Compiler* compiler) {
    Xvr_freeLiteralArray(&compiler->literalCache);
    XVR_FREE_ARRAY(unsigned char, compiler->bytecode, compiler->capacity);
    compiler->bytecode = NULL;
    compiler->capacity = 0;
    compiler->count = 0;
    compiler->panic = false;
}

static void emitByte(unsigned char** collationPtr, int* capacityPtr,
                     int* countPtr, unsigned char byte) {
    // grow the array
    if (*countPtr + 1 > *capacityPtr) {
        int oldCapacity = *capacityPtr;
        *capacityPtr = XVR_GROW_CAPACITY(*capacityPtr);
        *collationPtr = XVR_GROW_ARRAY(unsigned char, *collationPtr,
                                       oldCapacity, *capacityPtr);
    }

    // append to the collation
    (*collationPtr)[(*countPtr)++] = byte;
}

static void Xvr_emitShort(unsigned char** collationPtr, int* capacityPtr,
                          int* countPtr, unsigned short bytes) {
    char* ptr = (char*)&bytes;

    emitByte(collationPtr, capacityPtr, countPtr, *ptr);
    ptr++;
    emitByte(collationPtr, capacityPtr, countPtr, *ptr);
}

static void emitInt(unsigned char** collationPtr, int* capacityPtr,
                    int* countPtr, int bytes) {
    char* ptr = (char*)&bytes;

    emitByte(collationPtr, capacityPtr, countPtr, *ptr);
    ptr++;
    emitByte(collationPtr, capacityPtr, countPtr, *ptr);
    ptr++;
    emitByte(collationPtr, capacityPtr, countPtr, *ptr);
    ptr++;
    emitByte(collationPtr, capacityPtr, countPtr, *ptr);
}

static void emitFloat(unsigned char** collationPtr, int* capacityPtr,
                      int* countPtr, float bytes) {
    char* ptr = (char*)&bytes;

    emitByte(collationPtr, capacityPtr, countPtr, *ptr);
    ptr++;
    emitByte(collationPtr, capacityPtr, countPtr, *ptr);
    ptr++;
    emitByte(collationPtr, capacityPtr, countPtr, *ptr);
    ptr++;
    emitByte(collationPtr, capacityPtr, countPtr, *ptr);
}

// return the result
static unsigned char* collateCompilerHeaderOpt(Xvr_Compiler* compiler,
                                               size_t* size, bool embedHeader) {
    if (compiler->panic) {
        fprintf(stderr, XVR_CC_ERROR
                "[internal] Can't collate panicked compiler\n" XVR_CC_RESET);
        return NULL;
    }
    int capacity = XVR_GROW_CAPACITY(0);
    int count = 0;
    unsigned char* collation = XVR_ALLOCATE(unsigned char, capacity);

    // for the function-section at the end of the main-collation
    int fnIndex = 0;  // counts up for each fn
    int fnCapacity = XVR_GROW_CAPACITY(0);
    int fnCount = 0;
    unsigned char* fnCollation = XVR_ALLOCATE(unsigned char, fnCapacity);

    if (embedHeader) {
        // embed the header with version information
        emitByte(&collation, &capacity, &count, XVR_VERSION_MAJOR);
        emitByte(&collation, &capacity, &count, XVR_VERSION_MINOR);
        emitByte(&collation, &capacity, &count, XVR_VERSION_PATCH);

        // embed the build info
        if ((int)strlen(XVR_VERSION_BUILD) + count + 1 > capacity) {
            int oldCapacity = capacity;
            capacity =
                strlen(XVR_VERSION_BUILD) + count + 1;  // full header size
            collation =
                XVR_GROW_ARRAY(unsigned char, collation, oldCapacity, capacity);
        }

        memcpy(&collation[count], XVR_VERSION_BUILD, strlen(XVR_VERSION_BUILD));
        count += strlen(XVR_VERSION_BUILD);
        collation[count++] = '\0';  // terminate the build string

        emitByte(&collation, &capacity, &count,
                 XVR_OP_SECTION_END);  // terminate header
    }

    // embed the data section (first short is the number of literals)
    Xvr_emitShort(&collation, &capacity, &count, compiler->literalCache.count);

    // emit each literal by type
    for (int i = 0; i < compiler->literalCache.count; i++) {
        // literal Opcode
        //  emitShort(&collation, &capacity, &count, OP_LITERAL); //This isn't
        //  needed

        // literal type, followed by literal value
        switch (compiler->literalCache.literals[i].type) {
        case XVR_LITERAL_NULL:
            emitByte(&collation, &capacity, &count, XVR_LITERAL_NULL);
            // null has no following value
            break;

        case XVR_LITERAL_BOOLEAN:
            emitByte(&collation, &capacity, &count, XVR_LITERAL_BOOLEAN);
            emitByte(&collation, &capacity, &count,
                     XVR_AS_BOOLEAN(compiler->literalCache.literals[i]));
            break;

        case XVR_LITERAL_INTEGER:
            emitByte(&collation, &capacity, &count, XVR_LITERAL_INTEGER);
            emitInt(&collation, &capacity, &count,
                    XVR_AS_INTEGER(compiler->literalCache.literals[i]));
            break;

        case XVR_LITERAL_FLOAT:
            emitByte(&collation, &capacity, &count, XVR_LITERAL_FLOAT);
            emitFloat(&collation, &capacity, &count,
                      XVR_AS_FLOAT(compiler->literalCache.literals[i]));
            break;

        case XVR_LITERAL_STRING: {
            emitByte(&collation, &capacity, &count, XVR_LITERAL_STRING);

            Xvr_Literal str = compiler->literalCache.literals[i];

            for (int c = 0; c < (int)Xvr_lengthRefString(XVR_AS_STRING(str));
                 c++) {
                emitByte(&collation, &capacity, &count,
                         Xvr_toCString(XVR_AS_STRING(str))[c]);
            }

            emitByte(&collation, &capacity, &count,
                     '\0');  // terminate the string
        } break;

        case XVR_LITERAL_ARRAY: {
            emitByte(&collation, &capacity, &count, XVR_LITERAL_ARRAY);

            Xvr_LiteralArray* ptr =
                XVR_AS_ARRAY(compiler->literalCache.literals[i]);

            // length of the array, as a short
            Xvr_emitShort(&collation, &capacity, &count, ptr->count);

            // each element of the array
            for (int i = 0; i < ptr->count; i++) {
                Xvr_emitShort(
                    &collation, &capacity, &count,
                    (unsigned short)XVR_AS_INTEGER(
                        ptr->literals[i]));  // shorts representing the indexes
                                             // of the values
            }
        } break;

        case XVR_LITERAL_ARRAY_INTERMEDIATE: {
            emitByte(&collation, &capacity, &count,
                     XVR_LITERAL_ARRAY_INTERMEDIATE);

            Xvr_LiteralArray* ptr =
                XVR_AS_ARRAY(compiler->literalCache.literals[i]);

            // length of the array, as a short
            Xvr_emitShort(&collation, &capacity, &count, ptr->count);

            // each element of the array
            for (int i = 0; i < ptr->count; i++) {
                Xvr_emitShort(
                    &collation, &capacity, &count,
                    (unsigned short)XVR_AS_INTEGER(
                        ptr->literals[i]));  // shorts representing the indexes
                                             // of the values
            }
        } break;

        case XVR_LITERAL_DICTIONARY: {
            emitByte(&collation, &capacity, &count, XVR_LITERAL_DICTIONARY);

            Xvr_LiteralArray* ptr = XVR_AS_ARRAY(
                compiler->literalCache
                    .literals[i]);  // used an array for storage above

            // length of the array, as a short
            Xvr_emitShort(&collation, &capacity, &count,
                          ptr->count);  // count is the array size, NOT the
                                        // dictionary size

            // each element of the array
            for (int i = 0; i < ptr->count; i++) {
                Xvr_emitShort(
                    &collation, &capacity, &count,
                    (unsigned short)XVR_AS_INTEGER(
                        ptr->literals[i]));  // shorts representing the indexes
                                             // of the values
            }
        } break;

        case XVR_LITERAL_DICTIONARY_INTERMEDIATE: {
            emitByte(&collation, &capacity, &count,
                     XVR_LITERAL_DICTIONARY_INTERMEDIATE);

            Xvr_LiteralArray* ptr = XVR_AS_ARRAY(
                compiler->literalCache
                    .literals[i]);  // used an array for storage above

            // length of the array, as a short
            Xvr_emitShort(&collation, &capacity, &count,
                          ptr->count);  // count is the array size, NOT the
                                        // dictionary size

            // each element of the array
            for (int i = 0; i < ptr->count; i++) {
                Xvr_emitShort(
                    &collation, &capacity, &count,
                    (unsigned short)XVR_AS_INTEGER(
                        ptr->literals[i]));  // shorts representing the indexes
                                             // of the values
            }
        } break;

        case XVR_LITERAL_FUNCTION_INTERMEDIATE: {
            // extract the compiler
            Xvr_Literal fn = compiler->literalCache.literals[i];
            void* fnCompiler =
                XVR_AS_FUNCTION(fn)
                    .inner.bytecode;  // store the compiler here for now

            // collate the function into bytecode (without header)
            size_t size = 0;
            unsigned char* bytes = collateCompilerHeaderOpt(
                (Xvr_Compiler*)fnCompiler, &size, false);

            // emit how long this section is, +1 for ending mark
            Xvr_emitShort(&fnCollation, &fnCapacity, &fnCount,
                          (unsigned short)size + 1);

            // write the fn to the fn collation
            for (size_t i = 0; i < size; i++) {
                emitByte(&fnCollation, &fnCapacity, &fnCount, bytes[i]);
            }

            emitByte(&fnCollation, &fnCapacity, &fnCount,
                     XVR_OP_FN_END);  // for marking the correct end-point of
                                      // the function

            // embed the reference to the function implementation into the
            // current collation (to be extracted later)
            emitByte(&collation, &capacity, &count, XVR_LITERAL_FUNCTION);
            Xvr_emitShort(&collation, &capacity, &count,
                          (unsigned short)(fnIndex++));

            Xvr_freeCompiler((Xvr_Compiler*)fnCompiler);
            XVR_FREE(Xvr_Compiler, fnCompiler);
            XVR_FREE_ARRAY(unsigned char, bytes, size);
        } break;

        case XVR_LITERAL_IDENTIFIER: {
            emitByte(&collation, &capacity, &count, XVR_LITERAL_IDENTIFIER);

            Xvr_Literal identifier = compiler->literalCache.literals[i];

            for (int c = 0;
                 c < (int)Xvr_lengthRefString(XVR_AS_IDENTIFIER(identifier));
                 c++) {
                emitByte(&collation, &capacity, &count,
                         Xvr_toCString(XVR_AS_IDENTIFIER(identifier))[c]);
            }

            emitByte(&collation, &capacity, &count,
                     '\0');  // terminate the string
        } break;

        case XVR_LITERAL_TYPE: {
            // push a raw type
            emitByte(&collation, &capacity, &count, XVR_LITERAL_TYPE);

            Xvr_Literal typeLiteral = compiler->literalCache.literals[i];

            // what type this literal represents
            emitByte(&collation, &capacity, &count,
                     XVR_AS_TYPE(typeLiteral).typeOf);
            emitByte(&collation, &capacity, &count,
                     XVR_AS_TYPE(typeLiteral).constant);  // if it's constant
        } break;

        case XVR_LITERAL_TYPE_INTERMEDIATE: {
            emitByte(&collation, &capacity, &count,
                     XVR_LITERAL_TYPE_INTERMEDIATE);

            Xvr_LiteralArray* ptr = XVR_AS_ARRAY(
                compiler->literalCache
                    .literals[i]);  // used an array for storage above

            // the base literal
            Xvr_Literal typeLiteral = Xvr_copyLiteral(ptr->literals[0]);

            // what type this literal represents
            emitByte(&collation, &capacity, &count,
                     XVR_AS_TYPE(typeLiteral).typeOf);
            emitByte(&collation, &capacity, &count,
                     XVR_AS_TYPE(typeLiteral).constant);  // if it's constant

            if (XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_ARRAY ||
                XVR_AS_TYPE(typeLiteral).typeOf == XVR_LITERAL_DICTIONARY) {
                // the type will represent how many to expect in the array
                for (int i = 1; i < ptr->count; i++) {
                    Xvr_emitShort(
                        &collation, &capacity, &count,
                        (unsigned short)XVR_AS_INTEGER(
                            ptr->literals[i]));  // shorts representing the
                                                 // indexes of the types
                }
            }

            Xvr_freeLiteral(typeLiteral);
        } break;

        case XVR_LITERAL_INDEX_BLANK:
            emitByte(&collation, &capacity, &count, XVR_LITERAL_INDEX_BLANK);
            // blank has no following value
            break;

        default:
            fprintf(stderr,
                    XVR_CC_ERROR
                    "[internal] Unknown literal type encountered within "
                    "literal cache: %d\n" XVR_CC_RESET,
                    compiler->literalCache.literals[i].type);
            return NULL;
        }
    }

    emitByte(&collation, &capacity, &count,
             XVR_OP_SECTION_END);  // terminate data

    // embed the function section (beginning with function count, size)
    Xvr_emitShort(&collation, &capacity, &count, fnIndex);
    Xvr_emitShort(&collation, &capacity, &count, fnCount);

    for (int i = 0; i < fnCount; i++) {
        emitByte(&collation, &capacity, &count, fnCollation[i]);
    }

    emitByte(&collation, &capacity, &count,
             XVR_OP_SECTION_END);  // terminate function section

    XVR_FREE_ARRAY(unsigned char, fnCollation,
                   fnCapacity);  // clear the function stuff

    // code section
    for (int i = 0; i < compiler->count; i++) {
        emitByte(&collation, &capacity, &count, compiler->bytecode[i]);
    }

    emitByte(&collation, &capacity, &count,
             XVR_OP_SECTION_END);  // terminate code

    emitByte(&collation, &capacity, &count, XVR_OP_EOF);  // terminate bytecode

    // finalize
    collation = XVR_SHRINK_ARRAY(unsigned char, collation, capacity, count);

    *size = count;

    return collation;
}

unsigned char* Xvr_collateCompiler(Xvr_Compiler* compiler, size_t* size) {
    return collateCompilerHeaderOpt(compiler, size, true);
}
