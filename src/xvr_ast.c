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

#include "xvr_ast.h"
#include "xvr_bucket.h"

void Xvr_private_initAstBlock(Xvr_Bucket **bucketHandle, Xvr_Ast **handle) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_BLOCK;
  tmp->block.child = NULL;
  tmp->block.next = NULL;
  tmp->block.tail = NULL;

  (*handle) = tmp;
}

void Xvr_private_appendAstBlock(Xvr_Bucket **bucketHandle, Xvr_Ast *block,
                                Xvr_Ast *child) {
  // first, check if we're an empty head
  if (block->block.child == NULL) {
    block->block.child = child;
    return;
  }

  // run (or jump) until we hit the current tail
  Xvr_Ast *iter = block->block.tail ? block->block.tail : block;

  while (iter->block.next != NULL) {
    iter = iter->block.next;
  }

  // append a new link to the chain
  Xvr_private_initAstBlock(bucketHandle, &(iter->block.next));

  // store the child in the new link, prep the tail pointer
  iter->block.next->block.child = child;
  block->block.tail = iter->block.next;
}

void Xvr_private_emitAstValue(Xvr_Bucket **bucketHandle, Xvr_Ast **handle,
                              Xvr_Value value) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_VALUE;
  tmp->value.value = value;

  (*handle) = tmp;
}

void Xvr_private_emitAstUnary(Xvr_Bucket **bucketHandle, Xvr_Ast **handle,
                              Xvr_AstFlag flag) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_UNARY;
  tmp->unary.flag = flag;
  tmp->unary.child = *handle;

  (*handle) = tmp;
}

void Xvr_private_emitAstBinary(Xvr_Bucket **bucketHandle, Xvr_Ast **handle,
                               Xvr_AstFlag flag, Xvr_Ast *right) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_BINARY;
  tmp->binary.flag = flag;
  tmp->binary.left = *handle; // left-recursive
  tmp->binary.right = right;

  (*handle) = tmp;
}

void Xvr_private_emitAstCompare(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle,
                                Xvr_AstFlag flag, Xvr_Ast *right) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_COMPARE;
  tmp->compare.flag = flag;
  tmp->compare.left = *astHandle;
  tmp->compare.right = right;

  (*astHandle) = tmp;
}

void Xvr_private_emitAstGroup(Xvr_Bucket **bucketHandle, Xvr_Ast **handle) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_GROUP;
  tmp->group.child = (*handle);

  (*handle) = tmp;
}

void Xvr_private_emitAstCompound(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle,
                                 Xvr_AstFlag flag, Xvr_Ast *right) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_COMPOUND;
  tmp->compound.flag = flag;
  tmp->compound.left = *astHandle;
  tmp->compound.right = right;
  (*astHandle) = tmp;
}

void Xvr_private_emitAstAssert(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle,
                               Xvr_Ast *child, Xvr_Ast *msg) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_ASSERT;
  tmp->assert.child = child;
  tmp->assert.message = msg;
  (*astHandle) = tmp;
}

void Xvr_private_emitAstIfThenElse(Xvr_Bucket **bucketHandle,
                                   Xvr_Ast **astHandle, Xvr_Ast *condBranch,
                                   Xvr_Ast *thenBranch, Xvr_Ast *elseBranch) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_IF_THEN_ELSE;
  tmp->ifThenElse.condBranch = condBranch;
  tmp->ifThenElse.thenBranch = thenBranch;
  tmp->ifThenElse.elseBranch = elseBranch;

  (*astHandle) = tmp;
}

void Xvr_private_emitAstWhileThen(Xvr_Bucket **bucketHandle,
                                  Xvr_Ast **astHandle, Xvr_Ast *condBranch,
                                  Xvr_Ast *thenBranch) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_WHILE_THEN;
  tmp->whileThen.condBranch = condBranch;
  tmp->whileThen.thenBranch = thenBranch;
  (*astHandle) = tmp;
}

void Xvr_private_emitAstBreak(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle) {
  Xvr_Ast* tmp = (Xvr_Ast*)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));
  tmp->type = XVR_AST_BREAK;
  (*astHandle) = tmp;
}

void Xvr_private_emitAstContinue(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle) {
  Xvr_Ast* tmp = (Xvr_Ast*)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));
  tmp->type = XVR_AST_CONTINUE;
  (*astHandle) = tmp;
}

void Xvr_private_emitAstPrint(Xvr_Bucket **bucketHandle, Xvr_Ast **astHandle) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));
  tmp->type = XVR_AST_PRINT;
  tmp->print.child = (*astHandle);
  (*astHandle) = tmp;
}

void Xvr_private_emitAstVariableDeclaration(Xvr_Bucket **bucketHandle,
                                            Xvr_Ast **astHandle,
                                            Xvr_String *name, Xvr_Ast *expr) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_VAR_DECLARE;
  tmp->varDeclare.name = name;
  tmp->varDeclare.expr = expr;

  (*astHandle) = tmp;
}

void Xvr_private_emitAstVariableAssignment(Xvr_Bucket **bucketHandle,
                                           Xvr_Ast **astHandle,
                                           Xvr_String *name, Xvr_AstFlag flag,
                                           Xvr_Ast *expr) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_VAR_ASSIGN;
  tmp->varAssign.flag = flag;
  tmp->varAssign.name = name;
  tmp->varAssign.expr = expr;

  (*astHandle) = tmp;
}

void Xvr_private_emitAstVariableAccess(Xvr_Bucket **bucketHandle,
                                       Xvr_Ast **astHandle, Xvr_String *name) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_VAR_ACCESS;
  tmp->varAccess.name = name;
  (*astHandle) = tmp;
}

void Xvr_private_emitAstPass(Xvr_Bucket **bucketHandle, Xvr_Ast **handle) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_PASS;

  (*handle) = tmp;
}

void Xvr_private_emitAstError(Xvr_Bucket **bucketHandle, Xvr_Ast **handle) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_ERROR;

  (*handle) = tmp;
}

void Xvr_private_emitAstEnd(Xvr_Bucket **bucketHandle, Xvr_Ast **handle) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_END;

  (*handle) = tmp;
}
