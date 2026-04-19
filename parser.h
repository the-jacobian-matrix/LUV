#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include "lexer.h"

typedef enum {
  AST_MODULE,
  AST_DECLARATION,
  AST_STATEMENT,
  AST_EXPRESSION,
  AST_ERROR
} ASTNodeKind;

typedef struct ASTNode {
  ASTNodeKind kind;
  Token token;
  struct ASTNode *next;
} ASTNode;

typedef struct {
  Token *tokens;
  size_t token_count;
  size_t current;
  int panic_mode;
  int error_count;
} Parser;

void parser_init(Parser *p, Token *tokens, size_t token_count);

Token peek(Parser *p);
Token previous(Parser *p);
Token advance(Parser *p);
int check(Parser *p, TokenType type);
int match(Parser *p, TokenType type);
Token consume(Parser *p, TokenType type, const char *message);
int is_at_end(Parser *p);
void synchronize(Parser *p);

ASTNode *parse_module(Parser *p);

#endif
