#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"

typedef struct {
  Lexer lexer;
  Token current;
  Token previous;
  int had_error;
} Parser;

void parser_init(Parser *parser, const char *source);
Expr *parse_expression(Parser *parser);

#endif
