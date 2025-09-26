#include "xvr_ast.h"
#include "xvr_memory.h"

void Xvr_private_initAstBlock(Xvr_Bucket **bucket, Xvr_Ast **handle) {
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

  Xvr_private_initAstBlock(bucket, &(iter->block.next));

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
                              Xvr_AstFlag flag) {

  Xvr_Ast *temp = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  temp->unary.type = XVR_AST_UNARY;
  temp->unary.flag = flag;
  temp->unary.child = *handle;
  (*handle) = temp;
}

void Xvr_private_emitAstBinary(Xvr_Bucket **bucket, Xvr_Ast **handle,
                               Xvr_AstFlag flag, Xvr_Ast *right) {
  Xvr_Ast *temp = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  temp->binary.type = XVR_AST_BINARY;
  temp->binary.flag = flag;
  temp->binary.left = *handle;
  (*handle)->binary.right = right;
  (*handle) = temp;
}

void Xvr_private_emitAstGroup(Xvr_Bucket **bucket, Xvr_Ast **handle) {
  Xvr_Ast *temp = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  temp->group.type = XVR_AST_GROUP;
  temp->group.child = (*handle);
  (*handle) = temp;
}

void Xvr_private_emitAstPass(Xvr_Bucket **bucket, Xvr_Ast **handle) {
  (*handle) = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  (*handle)->pass.type = XVR_AST_PASS;
}

void Xvr_private_emitAstError(Xvr_Bucket **bucket, Xvr_Ast **handle) {
  (*handle) = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  (*handle)->error.type = XVR_AST_ERROR;
}

void Xvr_private_emitAstEnd(Xvr_Bucket **bucket, Xvr_Ast **handle) {
  (*handle) = (Xvr_Ast *)Xvr_partBucket(bucket, sizeof(Xvr_Ast));

  (*handle)->error.type = XVR_AST_END;
}
