#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

static ASTNode *make_node(ASTNodeKind kind, Token token) {
  ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
  if (!node) {
    fprintf(stderr, "Parser allocation failed.\n");
    exit(1);
  }
  node->kind = kind;
  node->token = token;
  return node;
}

static void parser_error_at(Parser *p, Token t, const char *message) {
  if (p->panic_mode)
    return;

  p->panic_mode = 1;
  p->error_count++;

  fprintf(stderr, "[line %d:%d] Parse error", t.line, t.column);
  if (t.type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else {
    fprintf(stderr, " at '%.*s'", (int)t.length, t.start);
  }
  fprintf(stderr, ": %s\n", message);
}

void parser_init(Parser *p, Token *tokens, size_t token_count) {
  p->tokens = tokens;
  p->token_count = token_count;
  p->current = 0;
  p->panic_mode = 0;
  p->error_count = 0;
}

int is_at_end(Parser *p) { return peek(p).type == TOKEN_EOF; }

Token peek(Parser *p) {
  if (p->token_count == 0) {
    Token t = {0};
    t.type = TOKEN_EOF;
    return t;
  }
  if (p->current >= p->token_count)
    return p->tokens[p->token_count - 1];
  return p->tokens[p->current];
}

Token previous(Parser *p) {
  if (p->current == 0)
    return peek(p);
  return p->tokens[p->current - 1];
}

Token advance(Parser *p) {
  if (!is_at_end(p) && p->current < p->token_count)
    p->current++;
  return previous(p);
}

int check(Parser *p, TokenType type) {
  if (is_at_end(p))
    return 0;
  return peek(p).type == type;
}

int match(Parser *p, TokenType type) {
  if (!check(p, type))
    return 0;
  advance(p);
  return 1;
}

Token consume(Parser *p, TokenType type, const char *message) {
  if (check(p, type))
    return advance(p);

  parser_error_at(p, peek(p), message);
  return peek(p);
}

void synchronize(Parser *p) {
  p->panic_mode = 0;

  while (!is_at_end(p)) {
    if (previous(p).type == TOKEN_SEMICOLON)
      return;

    switch (peek(p).type) {
    case TOKEN_MODULE:
    case TOKEN_USE:
    case TOKEN_FN:
    case TOKEN_CLASS:
    case TOKEN_IMPL:
    case TOKEN_MUT:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_FOR:
    case TOKEN_MATCH:
    case TOKEN_RETURN:
      return;
    default:
      break;
    }

    advance(p);
  }
}

static ASTNode *parse_expression(Parser *p);

static ASTNode *parse_primary(Parser *p) {
  if (match(p, TOKEN_VARDATA) || match(p, TOKEN_CHAR) || match(p, TOKEN_TRUE) ||
      match(p, TOKEN_FALSE) || match(p, TOKEN_IDENTIFIER) ||
      match(p, TOKEN_STRING_INTERP) || match(p, TOKEN_SELF) ||
      match(p, TOKEN_SUPER)) {
    return make_node(AST_EXPRESSION, previous(p));
  }

  if (match(p, TOKEN_LPAREN)) {
    ASTNode *expr = parse_expression(p);
    consume(p, TOKEN_RPAREN, "Expected ')' after grouped expression.");
    return expr;
  }

  parser_error_at(p, peek(p), "Expected expression.");
  return make_node(AST_ERROR, peek(p));
}

static ASTNode *parse_call(Parser *p) {
  ASTNode *expr = parse_primary(p);

  for (;;) {
    if (match(p, TOKEN_LPAREN)) {
      while (!check(p, TOKEN_RPAREN) && !is_at_end(p)) {
        parse_expression(p);
        if (!match(p, TOKEN_COMMA))
          break;
      }
      consume(p, TOKEN_RPAREN, "Expected ')' after arguments.");
    } else if (match(p, TOKEN_DOT)) {
      consume(p, TOKEN_IDENTIFIER, "Expected property name after '.'.");
    } else {
      break;
    }
  }

  return expr;
}

static ASTNode *parse_unary(Parser *p) {
  if (match(p, TOKEN_BANG) || match(p, TOKEN_MINUS) || match(p, TOKEN_TILDE)) {
    Token op = previous(p);
    parse_unary(p);
    return make_node(AST_EXPRESSION, op);
  }

  return parse_call(p);
}

static ASTNode *parse_factor(Parser *p) {
  ASTNode *expr = parse_unary(p);

  while (match(p, TOKEN_STAR) || match(p, TOKEN_SLASH) || match(p, TOKEN_PERCENT)) {
    Token op = previous(p);
    parse_unary(p);
    expr = make_node(AST_EXPRESSION, op);
  }

  return expr;
}

static ASTNode *parse_term(Parser *p) {
  ASTNode *expr = parse_factor(p);

  while (match(p, TOKEN_PLUS) || match(p, TOKEN_MINUS)) {
    Token op = previous(p);
    parse_factor(p);
    expr = make_node(AST_EXPRESSION, op);
  }

  return expr;
}

static ASTNode *parse_comparison(Parser *p) {
  ASTNode *expr = parse_term(p);

  while (match(p, TOKEN_GREATER) || match(p, TOKEN_GREATER_EQUAL) ||
         match(p, TOKEN_LESS) || match(p, TOKEN_LESS_EQUAL)) {
    Token op = previous(p);
    parse_term(p);
    expr = make_node(AST_EXPRESSION, op);
  }

  return expr;
}

static ASTNode *parse_equality(Parser *p) {
  ASTNode *expr = parse_comparison(p);

  while (match(p, TOKEN_EQUAL_EQUAL) || match(p, TOKEN_BANG_EQUAL)) {
    Token op = previous(p);
    parse_comparison(p);
    expr = make_node(AST_EXPRESSION, op);
  }

  return expr;
}

static ASTNode *parse_and(Parser *p) {
  ASTNode *expr = parse_equality(p);

  while (match(p, TOKEN_AND_AND)) {
    Token op = previous(p);
    parse_equality(p);
    expr = make_node(AST_EXPRESSION, op);
  }

  return expr;
}

static ASTNode *parse_or(Parser *p) {
  ASTNode *expr = parse_and(p);

  while (match(p, TOKEN_OR_OR)) {
    Token op = previous(p);
    parse_and(p);
    expr = make_node(AST_EXPRESSION, op);
  }

  return expr;
}

static ASTNode *parse_assignment(Parser *p) {
  ASTNode *expr = parse_or(p);

  if (match(p, TOKEN_EQUAL) || match(p, TOKEN_PLUS_EQUAL) ||
      match(p, TOKEN_MINUS_EQUAL) || match(p, TOKEN_STAR_EQUAL) ||
      match(p, TOKEN_SLASH_EQUAL) || match(p, TOKEN_PERCENT_EQUAL)) {
    Token op = previous(p);
    parse_assignment(p);
    expr = make_node(AST_EXPRESSION, op);
  }

  return expr;
}

static ASTNode *parse_expression(Parser *p) { return parse_assignment(p); }

static ASTNode *parse_block(Parser *p) {
  Token start = previous(p);

  while (!check(p, TOKEN_RBRACE) && !is_at_end(p)) {
    parse_expression(p);
    match(p, TOKEN_SEMICOLON);
  }

  consume(p, TOKEN_RBRACE, "Expected '}' after block.");
  return make_node(AST_STATEMENT, start);
}

static ASTNode *parse_statement(Parser *p) {
  if (match(p, TOKEN_IF) || match(p, TOKEN_EF) || match(p, TOKEN_ELSE)) {
    Token keyword = previous(p);
    if (keyword.type != TOKEN_ELSE) {
      parse_expression(p);
    }
    if (match(p, TOKEN_LBRACE)) {
      parse_block(p);
    } else {
      parse_expression(p);
      match(p, TOKEN_SEMICOLON);
    }
    return make_node(AST_STATEMENT, keyword);
  }

  if (match(p, TOKEN_WHILE) || match(p, TOKEN_FOR) || match(p, TOKEN_MATCH)) {
    Token keyword = previous(p);
    parse_expression(p);
    if (match(p, TOKEN_LBRACE)) {
      parse_block(p);
    } else {
      consume(p, TOKEN_SEMICOLON, "Expected ';' after loop/match statement.");
    }
    return make_node(AST_STATEMENT, keyword);
  }

  if (match(p, TOKEN_RETURN)) {
    Token keyword = previous(p);
    if (!check(p, TOKEN_SEMICOLON)) {
      parse_expression(p);
    }
    consume(p, TOKEN_SEMICOLON, "Expected ';' after return value.");
    return make_node(AST_STATEMENT, keyword);
  }

  if (match(p, TOKEN_LBRACE)) {
    return parse_block(p);
  }

  ASTNode *expr = parse_expression(p);
  consume(p, TOKEN_SEMICOLON, "Expected ';' after expression statement.");
  return expr;
}

static ASTNode *parse_declaration(Parser *p) {
  if (match(p, TOKEN_MODULE) || match(p, TOKEN_USE) || match(p, TOKEN_FN) ||
      match(p, TOKEN_CLASS) || match(p, TOKEN_IMPL) || match(p, TOKEN_MUT)) {
    Token keyword = previous(p);

    while (!check(p, TOKEN_SEMICOLON) && !check(p, TOKEN_LBRACE) &&
           !is_at_end(p)) {
      advance(p);
    }

    if (match(p, TOKEN_LBRACE)) {
      parse_block(p);
    } else {
      consume(p, TOKEN_SEMICOLON, "Expected ';' after declaration.");
    }

    return make_node(AST_DECLARATION, keyword);
  }

  return parse_statement(p);
}

ASTNode *parse_module(Parser *p) {
  Token module_token = peek(p);
  ASTNode *head = make_node(AST_MODULE, module_token);
  ASTNode *tail = head;

  while (!is_at_end(p)) {
    ASTNode *node = parse_declaration(p);
    tail->next = node;
    tail = node;

    if (p->panic_mode)
      synchronize(p);
  }

  return head;
}
