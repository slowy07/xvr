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

void Xvr_private_emitAstGroup(Xvr_Bucket **bucketHandle, Xvr_Ast **handle) {
  Xvr_Ast *tmp = (Xvr_Ast *)Xvr_partitionBucket(bucketHandle, sizeof(Xvr_Ast));

  tmp->type = XVR_AST_GROUP;
  tmp->group.child = (*handle);

  (*handle) = tmp;
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
