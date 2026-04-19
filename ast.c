#include "ast.h"
#include <stdlib.h>

static Expr *alloc_expr(ExprKind kind) {
  Expr *expr = (Expr *)malloc(sizeof(Expr));
  if (!expr)
    return NULL;
  expr->kind = kind;
  return expr;
}

Expr *expr_new_literal(Token token) {
  Expr *expr = alloc_expr(EXPR_LITERAL);
  if (!expr)
    return NULL;
  expr->as.literal = token;
  return expr;
}

Expr *expr_new_identifier(Token token) {
  Expr *expr = alloc_expr(EXPR_IDENTIFIER);
  if (!expr)
    return NULL;
  expr->as.identifier = token;
  return expr;
}

Expr *expr_new_grouping(Expr *inner) {
  Expr *expr = alloc_expr(EXPR_GROUPING);
  if (!expr)
    return NULL;
  expr->as.grouping.expr = inner;
  return expr;
}

Expr *expr_new_unary(Token op, Expr *right) {
  Expr *expr = alloc_expr(EXPR_UNARY);
  if (!expr)
    return NULL;
  expr->as.unary.op = op;
  expr->as.unary.right = right;
  return expr;
}

Expr *expr_new_binary(Expr *left, Token op, Expr *right) {
  Expr *expr = alloc_expr(EXPR_BINARY);
  if (!expr)
    return NULL;
  expr->as.binary.left = left;
  expr->as.binary.op = op;
  expr->as.binary.right = right;
  return expr;
}

Expr *expr_new_assignment(Expr *target, AssignOp op, Expr *value) {
  Expr *expr = alloc_expr(EXPR_ASSIGNMENT);
  if (!expr)
    return NULL;
  expr->as.assignment.target = target;
  expr->as.assignment.op = op;
  expr->as.assignment.value = value;
  return expr;
}

Expr *expr_new_call(Expr *callee, Expr **args, size_t arg_count) {
  Expr *expr = alloc_expr(EXPR_CALL);
  if (!expr)
    return NULL;
  expr->as.call.callee = callee;
  expr->as.call.args = args;
  expr->as.call.arg_count = arg_count;
  return expr;
}

Expr *expr_new_member(Expr *object, Token member) {
  Expr *expr = alloc_expr(EXPR_MEMBER);
  if (!expr)
    return NULL;
  expr->as.member.object = object;
  expr->as.member.member = member;
  return expr;
}

Expr *expr_new_index(Expr *object, Expr *index) {
  Expr *expr = alloc_expr(EXPR_INDEX);
  if (!expr)
    return NULL;
  expr->as.index.object = object;
  expr->as.index.index = index;
  return expr;
}

Expr *expr_new_range(Expr *start, Expr *end, int inclusive) {
  Expr *expr = alloc_expr(EXPR_RANGE);
  if (!expr)
    return NULL;
  expr->as.range.start = start;
  expr->as.range.end = end;
  expr->as.range.inclusive = inclusive;
  return expr;
}

void expr_free(Expr *expr) {
  if (!expr)
    return;

  switch (expr->kind) {
  case EXPR_LITERAL:
  case EXPR_IDENTIFIER:
    break;
  case EXPR_GROUPING:
    expr_free(expr->as.grouping.expr);
    break;
  case EXPR_UNARY:
    expr_free(expr->as.unary.right);
    break;
  case EXPR_BINARY:
    expr_free(expr->as.binary.left);
    expr_free(expr->as.binary.right);
    break;
  case EXPR_ASSIGNMENT:
    expr_free(expr->as.assignment.target);
    expr_free(expr->as.assignment.value);
    break;
  case EXPR_CALL:
    expr_free(expr->as.call.callee);
    for (size_t i = 0; i < expr->as.call.arg_count; i++) {
      expr_free(expr->as.call.args[i]);
    }
    free(expr->as.call.args);
    break;
  case EXPR_MEMBER:
    expr_free(expr->as.member.object);
    break;
  case EXPR_INDEX:
    expr_free(expr->as.index.object);
    expr_free(expr->as.index.index);
    break;
  case EXPR_RANGE:
    expr_free(expr->as.range.start);
    expr_free(expr->as.range.end);
    break;
  }

  free(expr);
}
