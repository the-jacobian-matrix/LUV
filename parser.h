#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include "lexer.h"

typedef enum {
  AST_PROGRAM,
  AST_FUNCTION_DECL,
  AST_BLOCK,
  AST_RETURN_STMT,
  AST_CALL_EXPR,
  AST_IDENTIFIER,
  AST_LITERAL
} AstKind;

typedef struct AstNode AstNode;

typedef struct {
  size_t index;
} ParserMark;

typedef struct {
  Token *tokens;
  size_t count;
  size_t current;
} Parser;

struct AstNode {
  AstKind kind;
  Token token;
  union {
    struct {
      AstNode **items;
      size_t count;
    } list;

    struct {
      Token name;
      Token *params;
      size_t param_count;
      AstNode *body;
    } fn_decl;

    struct {
      AstNode **statements;
      size_t count;
    } block;

    struct {
      AstNode *value;
    } return_stmt;

    struct {
      AstNode *callee;
      AstNode **args;
      size_t arg_count;
    } call;
  } as;
};

void parser_init(Parser *p, Token *tokens, size_t count);
ParserMark parser_mark(Parser *p);
void parser_rewind(Parser *p, ParserMark mark);

AstNode *parse_declaration(Parser *p);
AstNode *parse_expression(Parser *p);
AstNode *parse_program(Parser *p);

#endif
