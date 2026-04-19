#include "parser.h"
#include <stdio.h>
#include <stdlib.h>

static void advance_parser(Parser *parser);
static int match_token(Parser *parser, TokenType type);
static int check_token(Parser *parser, TokenType type);
static int consume_token(Parser *parser, TokenType type, const char *message);
static void parser_error(Parser *parser, const char *message);

static Expr *parse_assignment(Parser *parser);
static Expr *parse_logical_or(Parser *parser);
static Expr *parse_logical_and(Parser *parser);
static Expr *parse_bitwise_or(Parser *parser);
static Expr *parse_bitwise_xor(Parser *parser);
static Expr *parse_bitwise_and(Parser *parser);
static Expr *parse_bitwise_shift(Parser *parser);
static Expr *parse_equality(Parser *parser);
static Expr *parse_comparison(Parser *parser);
static Expr *parse_range(Parser *parser);
static Expr *parse_additive(Parser *parser);
static Expr *parse_multiplicative(Parser *parser);
static Expr *parse_unary(Parser *parser);
static Expr *parse_postfix(Parser *parser);
static Expr *parse_primary(Parser *parser);

void parser_init(Parser *parser, const char *source) {
  lexer_init(&parser->lexer, source);
  parser->had_error = 0;
  parser->previous = (Token){0};
  parser->current = next_token(&parser->lexer);
}

Expr *parse_expression(Parser *parser) { return parse_assignment(parser); }

static void advance_parser(Parser *parser) {
  parser->previous = parser->current;
  parser->current = next_token(&parser->lexer);
}

static int match_token(Parser *parser, TokenType type) {
  if (!check_token(parser, type))
    return 0;
  advance_parser(parser);
  return 1;
}

static int check_token(Parser *parser, TokenType type) {
  return parser->current.type == type;
}

static int consume_token(Parser *parser, TokenType type, const char *message) {
  if (check_token(parser, type)) {
    advance_parser(parser);
    return 1;
  }
  parser_error(parser, message);
  return 0;
}

static void parser_error(Parser *parser, const char *message) {
  parser->had_error = 1;
  fprintf(stderr, "[parse error] line %d col %d: %s\n", parser->current.line,
          parser->current.column, message);
}

static Expr *parse_assignment(Parser *parser) {
  Expr *target = parse_logical_or(parser);

  AssignOp assign_op;
  int has_assignment = 1;
  if (match_token(parser, TOKEN_EQUAL)) {
    assign_op = ASSIGN_EQUAL;
  } else if (match_token(parser, TOKEN_PLUS_EQUAL)) {
    assign_op = ASSIGN_PLUS_EQUAL;
  } else if (match_token(parser, TOKEN_MINUS_EQUAL)) {
    assign_op = ASSIGN_MINUS_EQUAL;
  } else if (match_token(parser, TOKEN_STAR_EQUAL)) {
    assign_op = ASSIGN_STAR_EQUAL;
  } else if (match_token(parser, TOKEN_SLASH_EQUAL)) {
    assign_op = ASSIGN_SLASH_EQUAL;
  } else if (match_token(parser, TOKEN_PERCENT_EQUAL)) {
    assign_op = ASSIGN_PERCENT_EQUAL;
  } else {
    has_assignment = 0;
  }

  if (!has_assignment)
    return target;

  if (target->kind != EXPR_IDENTIFIER && target->kind != EXPR_MEMBER &&
      target->kind != EXPR_INDEX) {
    parser_error(parser, "Invalid assignment target.");
  }

  Expr *value = parse_assignment(parser);
  return expr_new_assignment(target, assign_op, value);
}

static Expr *parse_logical_or(Parser *parser) {
  Expr *expr = parse_logical_and(parser);
  while (match_token(parser, TOKEN_OR_OR)) {
    Token op = parser->previous;
    Expr *right = parse_logical_and(parser);
    expr = expr_new_binary(expr, op, right);
  }
  return expr;
}

static Expr *parse_logical_and(Parser *parser) {
  Expr *expr = parse_bitwise_or(parser);
  while (match_token(parser, TOKEN_AND_AND)) {
    Token op = parser->previous;
    Expr *right = parse_bitwise_or(parser);
    expr = expr_new_binary(expr, op, right);
  }
  return expr;
}

static Expr *parse_bitwise_or(Parser *parser) {
  Expr *expr = parse_bitwise_xor(parser);
  while (match_token(parser, TOKEN_PIPE)) {
    Token op = parser->previous;
    Expr *right = parse_bitwise_xor(parser);
    expr = expr_new_binary(expr, op, right);
  }
  return expr;
}

static Expr *parse_bitwise_xor(Parser *parser) {
  Expr *expr = parse_bitwise_and(parser);
  while (match_token(parser, TOKEN_CARET)) {
    Token op = parser->previous;
    Expr *right = parse_bitwise_and(parser);
    expr = expr_new_binary(expr, op, right);
  }
  return expr;
}

static Expr *parse_bitwise_and(Parser *parser) {
  Expr *expr = parse_bitwise_shift(parser);
  while (match_token(parser, TOKEN_AMP)) {
    Token op = parser->previous;
    Expr *right = parse_bitwise_shift(parser);
    expr = expr_new_binary(expr, op, right);
  }
  return expr;
}

static Expr *parse_bitwise_shift(Parser *parser) {
  Expr *expr = parse_equality(parser);
  while (match_token(parser, TOKEN_LEFT_SHIFT) ||
         match_token(parser, TOKEN_RIGHT_SHIFT)) {
    Token op = parser->previous;
    Expr *right = parse_equality(parser);
    expr = expr_new_binary(expr, op, right);
  }
  return expr;
}

static Expr *parse_equality(Parser *parser) {
  Expr *expr = parse_comparison(parser);
  while (match_token(parser, TOKEN_EQUAL_EQUAL) ||
         match_token(parser, TOKEN_BANG_EQUAL)) {
    Token op = parser->previous;
    Expr *right = parse_comparison(parser);
    expr = expr_new_binary(expr, op, right);
  }
  return expr;
}

static Expr *parse_comparison(Parser *parser) {
  Expr *expr = parse_range(parser);
  while (match_token(parser, TOKEN_LESS) || match_token(parser, TOKEN_LESS_EQUAL) ||
         match_token(parser, TOKEN_GREATER) ||
         match_token(parser, TOKEN_GREATER_EQUAL)) {
    Token op = parser->previous;
    Expr *right = parse_range(parser);
    expr = expr_new_binary(expr, op, right);
  }
  return expr;
}

static Expr *parse_range(Parser *parser) {
  Expr *start = parse_additive(parser);

  if (match_token(parser, TOKEN_DOT_DOT) ||
      match_token(parser, TOKEN_DOT_DOT_DOT)) {
    Token op = parser->previous;
    Expr *end = parse_additive(parser);
    return expr_new_range(start, end, op.type == TOKEN_DOT_DOT_DOT);
  }

  return start;
}

static Expr *parse_additive(Parser *parser) {
  Expr *expr = parse_multiplicative(parser);
  while (match_token(parser, TOKEN_PLUS) || match_token(parser, TOKEN_MINUS)) {
    Token op = parser->previous;
    Expr *right = parse_multiplicative(parser);
    expr = expr_new_binary(expr, op, right);
  }
  return expr;
}

static Expr *parse_multiplicative(Parser *parser) {
  Expr *expr = parse_unary(parser);
  while (match_token(parser, TOKEN_STAR) || match_token(parser, TOKEN_SLASH) ||
         match_token(parser, TOKEN_PERCENT)) {
    Token op = parser->previous;
    Expr *right = parse_unary(parser);
    expr = expr_new_binary(expr, op, right);
  }
  return expr;
}

static Expr *parse_unary(Parser *parser) {
  if (match_token(parser, TOKEN_BANG) || match_token(parser, TOKEN_MINUS) ||
      match_token(parser, TOKEN_PLUS) || match_token(parser, TOKEN_TILDE)) {
    Token op = parser->previous;
    Expr *right = parse_unary(parser);
    return expr_new_unary(op, right);
  }
  return parse_postfix(parser);
}

static Expr *parse_postfix(Parser *parser) {
  Expr *expr = parse_primary(parser);

  for (;;) {
    if (match_token(parser, TOKEN_LPAREN)) {
      Expr **args = NULL;
      size_t arg_count = 0;

      if (!check_token(parser, TOKEN_RPAREN)) {
        do {
          Expr *arg = parse_expression(parser);
          Expr **new_args =
              (Expr **)realloc(args, sizeof(Expr *) * (arg_count + 1));
          if (!new_args) {
            free(args);
            parser_error(parser, "Out of memory while parsing arguments.");
            return expr;
          }
          args = new_args;
          args[arg_count++] = arg;
        } while (match_token(parser, TOKEN_COMMA));
      }

      consume_token(parser, TOKEN_RPAREN, "Expected ')' after arguments.");
      expr = expr_new_call(expr, args, arg_count);
      continue;
    }

    if (match_token(parser, TOKEN_DOT)) {
      if (!consume_token(parser, TOKEN_IDENTIFIER,
                         "Expected member name after '.'.")) {
        return expr;
      }
      expr = expr_new_member(expr, parser->previous);
      continue;
    }

    if (match_token(parser, TOKEN_LBRACKET)) {
      Expr *index = parse_expression(parser);
      consume_token(parser, TOKEN_RBRACKET, "Expected ']' after index.");
      expr = expr_new_index(expr, index);
      continue;
    }

    break;
  }

  return expr;
}

static Expr *parse_primary(Parser *parser) {
  if (match_token(parser, TOKEN_IDENTIFIER)) {
    return expr_new_identifier(parser->previous);
  }

  if (match_token(parser, TOKEN_VARDATA) || match_token(parser, TOKEN_CHAR) ||
      match_token(parser, TOKEN_STRING_INTERP) ||
      match_token(parser, TOKEN_TRUE) || match_token(parser, TOKEN_FALSE)) {
    return expr_new_literal(parser->previous);
  }

  if (match_token(parser, TOKEN_LPAREN)) {
    Expr *inner = parse_expression(parser);
    consume_token(parser, TOKEN_RPAREN, "Expected ')' after expression.");
    return expr_new_grouping(inner);
  }

  parser_error(parser, "Expected expression.");
  return expr_new_literal((Token){.type = TOKEN_ERROR,
                                  .start = "<error>",
                                  .length = 7,
                                  .line = parser->current.line,
                                  .column = parser->current.column});
}
