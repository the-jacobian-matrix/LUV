#include "parser.h"
#include <stdlib.h>
#include <string.h>

static AstNode *new_node(AstKind kind, Token token) {
  AstNode *n = (AstNode *)calloc(1, sizeof(AstNode));
  n->kind = kind;
  n->token = token;
  return n;
}

static Token peek(Parser *p) { return p->tokens[p->current]; }

static Token previous(Parser *p) { return p->tokens[p->current - 1]; }

static int at_end(Parser *p) { return peek(p).type == TOKEN_EOF; }

static Token advance(Parser *p) {
  if (!at_end(p))
    p->current++;
  return previous(p);
}

static int check(Parser *p, TokenType type) {
  if (at_end(p))
    return type == TOKEN_EOF;
  return peek(p).type == type;
}

static int match(Parser *p, TokenType type) {
  if (!check(p, type))
    return 0;
  advance(p);
  return 1;
}

static Token consume(Parser *p, TokenType type) {
  if (check(p, type))
    return advance(p);
  return peek(p);
}

static void push_node(AstNode ***arr, size_t *count, AstNode *node) {
  *arr = (AstNode **)realloc(*arr, (*count + 1) * sizeof(AstNode *));
  (*arr)[(*count)++] = node;
}

static void push_token(Token **arr, size_t *count, Token token) {
  *arr = (Token *)realloc(*arr, (*count + 1) * sizeof(Token));
  (*arr)[(*count)++] = token;
}

void parser_init(Parser *p, Token *tokens, size_t count) {
  p->tokens = tokens;
  p->count = count;
  p->current = 0;
}

ParserMark parser_mark(Parser *p) {
  ParserMark m;
  m.index = p->current;
  return m;
}

void parser_rewind(Parser *p, ParserMark mark) { p->current = mark.index; }

static AstNode *parse_expression_internal(Parser *p);

static AstNode *parse_primary(Parser *p) {
  if (match(p, TOKEN_IDENTIFIER))
    return new_node(AST_IDENTIFIER, previous(p));

  if (match(p, TOKEN_VARDATA) || match(p, TOKEN_TRUE) || match(p, TOKEN_FALSE) ||
      match(p, TOKEN_CHAR) || match(p, TOKEN_STRING_INTERP)) {
    return new_node(AST_LITERAL, previous(p));
  }

  if (match(p, TOKEN_LPAREN)) {
    AstNode *expr = parse_expression_internal(p);
    consume(p, TOKEN_RPAREN);
    return expr;
  }

  return new_node(AST_LITERAL, peek(p));
}

static AstNode *finish_call(Parser *p, AstNode *callee) {
  Token lparen = previous(p);
  AstNode *call = new_node(AST_CALL_EXPR, lparen);
  call->as.call.callee = callee;

  if (!check(p, TOKEN_RPAREN)) {
    do {
      push_node(&call->as.call.args, &call->as.call.arg_count,
                parse_expression_internal(p));
    } while (match(p, TOKEN_COMMA));
  }

  consume(p, TOKEN_RPAREN);
  return call;
}

static AstNode *parse_call(Parser *p) {
  AstNode *expr = parse_primary(p);

  while (match(p, TOKEN_LPAREN)) {
    expr = finish_call(p, expr);
  }

  return expr;
}

static AstNode *parse_expression_internal(Parser *p) { return parse_call(p); }

AstNode *parse_expression(Parser *p) { return parse_expression_internal(p); }

static int try_parse_parameter_shape(Parser *p, Token **params, size_t *param_count) {
  if (!match(p, TOKEN_LPAREN))
    return 0;

  if (!check(p, TOKEN_RPAREN)) {
    do {
      if (!check(p, TOKEN_IDENTIFIER))
        return 0;
      push_token(params, param_count, advance(p));
    } while (match(p, TOKEN_COMMA));
  }

  return match(p, TOKEN_RPAREN);
}

static AstNode *parse_block(Parser *p) {
  Token block_tok = previous(p);
  AstNode *block = new_node(AST_BLOCK, block_tok);

  while (!check(p, TOKEN_RBRACE) && !at_end(p)) {
    AstNode *stmt = parse_declaration(p);
    if (stmt)
      push_node(&block->as.block.statements, &block->as.block.count, stmt);
    else
      advance(p);
  }

  consume(p, TOKEN_RBRACE);
  return block;
}

static AstNode *make_implicit_return_block(AstNode *expr) {
  AstNode *block = new_node(AST_BLOCK, expr->token);
  AstNode *ret = new_node(AST_RETURN_STMT, expr->token);
  ret->as.return_stmt.value = expr;
  push_node(&block->as.block.statements, &block->as.block.count, ret);
  return block;
}

static AstNode *parse_named_fn_declaration(Parser *p, Token name, Token *params,
                                           size_t param_count) {
  AstNode *fn = new_node(AST_FUNCTION_DECL, name);
  fn->as.fn_decl.name = name;
  fn->as.fn_decl.params = params;
  fn->as.fn_decl.param_count = param_count;

  if (match(p, TOKEN_THIN_ARROW)) {
    AstNode *expr_body = parse_expression_internal(p);
    fn->as.fn_decl.body = make_implicit_return_block(expr_body);
    return fn;
  }

  consume(p, TOKEN_LBRACE);
  fn->as.fn_decl.body = parse_block(p);
  return fn;
}

AstNode *parse_declaration(Parser *p) {
  if (check(p, TOKEN_IDENTIFIER)) {
    ParserMark m = parser_mark(p);
    Token name = advance(p);

    if (check(p, TOKEN_LPAREN)) {
      Token *params = NULL;
      size_t param_count = 0;
      if (try_parse_parameter_shape(p, &params, &param_count)) {
        if (check(p, TOKEN_THIN_ARROW) || check(p, TOKEN_LBRACE)) {
          return parse_named_fn_declaration(p, name, params, param_count);
        }
      }
      free(params);
    }

    parser_rewind(p, m);
  }

  AstNode *expr = parse_expression_internal(p);
  match(p, TOKEN_SEMICOLON);
  return expr;
}

AstNode *parse_program(Parser *p) {
  AstNode *program = new_node(AST_PROGRAM, peek(p));

  while (!at_end(p)) {
    AstNode *decl = parse_declaration(p);
    if (decl)
      push_node(&program->as.list.items, &program->as.list.count, decl);
    else
      advance(p);
  }

  return program;
}
