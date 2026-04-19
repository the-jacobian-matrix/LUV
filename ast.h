#ifndef AST_H
#define AST_H

#include <stddef.h>
#include "lexer.h"

typedef struct Expr Expr;

typedef enum {
  EXPR_LITERAL,
  EXPR_IDENTIFIER,
  EXPR_GROUPING,
  EXPR_UNARY,
  EXPR_BINARY,
  EXPR_ASSIGNMENT,
  EXPR_CALL,
  EXPR_MEMBER,
  EXPR_INDEX,
  EXPR_RANGE
} ExprKind;

typedef enum {
  ASSIGN_EQUAL,
  ASSIGN_PLUS_EQUAL,
  ASSIGN_MINUS_EQUAL,
  ASSIGN_STAR_EQUAL,
  ASSIGN_SLASH_EQUAL,
  ASSIGN_PERCENT_EQUAL
} AssignOp;

struct Expr {
  ExprKind kind;
  union {
    Token literal;
    Token identifier;
    struct {
      Expr *expr;
    } grouping;
    struct {
      Token op;
      Expr *right;
    } unary;
    struct {
      Expr *left;
      Token op;
      Expr *right;
    } binary;
    struct {
      Expr *target;
      AssignOp op;
      Expr *value;
    } assignment;
    struct {
      Expr *callee;
      Expr **args;
      size_t arg_count;
    } call;
    struct {
      Expr *object;
      Token member;
    } member;
    struct {
      Expr *object;
      Expr *index;
    } index;
    struct {
      Expr *start;
      Expr *end;
      int inclusive;
    } range;
  } as;
};

Expr *expr_new_literal(Token token);
Expr *expr_new_identifier(Token token);
Expr *expr_new_grouping(Expr *expr);
Expr *expr_new_unary(Token op, Expr *right);
Expr *expr_new_binary(Expr *left, Token op, Expr *right);
Expr *expr_new_assignment(Expr *target, AssignOp op, Expr *value);
Expr *expr_new_call(Expr *callee, Expr **args, size_t arg_count);
Expr *expr_new_member(Expr *object, Token member);
Expr *expr_new_index(Expr *object, Expr *index);
Expr *expr_new_range(Expr *start, Expr *end, int inclusive);

void expr_free(Expr *expr);

#endif
