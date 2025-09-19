#include "xvr_ast.h"
#include "xvr_memory.h"

void Xvr_private_initAsBlock(Xvr_Bucket **bucket, Xvr_Ast **handle) {
  (*handle) = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  (*handle)->block.type = XVR_AST_BLOCK;
  (*handle)->block.child = NULL;
  (*handle)->block.next = NULL;
  (*handle)->block.tail = NULL;
}

void Xvr_private_appendAstBlock(Xvr_Bucket **bucket, Xvr_Ast **handle,
                                Xvr_Ast *child) {
  if ((*handle)->block.child == NULL) {
    (*handle)->block.child = child;
    return;
  }

  Xvr_Ast *iter = (*handle)->block.tail ? (*handle)->block.tail : (*handle);

  while (iter->block.next != NULL) {
    iter = iter->block.next;
  }

  Xvr_private_initAsBlock(bucket, &(iter->block.next));

  iter->block.next->block.child = child;
  (*handle)->block.tail = iter->block.next;
}

void Xvr_private_emitAstValue(Xvr_Bucket **bucket, Xvr_Ast **handle,
                              Xvr_Value value) {
  (*handle) = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  (*handle)->unary.type = XVR_AST_VALUE;
  (*handle)->value.value = value;
}

void Xvr_private_emitAstUnary(Xvr_Bucket **bucket, Xvr_Ast **handle,
                              Xvr_AstFlag flag, Xvr_Ast *child) {
  (*handle) = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  (*handle)->unary.type = XVR_AST_UNARY;
  (*handle)->unary.flag = flag;
  (*handle)->unary.child = child;
}

void Xvr_private_emitAstBinary(Xvr_Bucket **bucket, Xvr_Ast **handle,
                               Xvr_AstFlag flag, Xvr_Ast *left,
                               Xvr_Ast *right) {
  (*handle) = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  (*handle)->binary.type = XVR_AST_BINARY;
  (*handle)->binary.flag = flag;
  (*handle)->binary.left = left;
  (*handle)->binary.right = right;
}

void Xvr_private_emitAstGroup(Xvr_Bucket **bucket, Xvr_Ast **handle,
                              Xvr_Ast *child) {
  (*handle) = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  (*handle)->group.type = XVR_AST_GROUP;
  (*handle)->group.child = child;
}

void Xvr_private_emitAstPass(Xvr_Bucket **bucket, Xvr_Ast **handle) {
  (*handle) = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  (*handle)->pass.type = XVR_AST_PASS;
}

void Xvr_private_emitAstError(Xvr_Bucket **bucket, Xvr_Ast **handle) {
  (*handle) = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  (*handle)->error.type = XVR_AST_ERROR;
}
