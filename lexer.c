#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =========================================================
   KEYWORDS
   ========================================================= */

typedef struct {
  const char *text;
  TokenType type;
} Keyword;

static Keyword keywords[] = {{"true", TOKEN_TRUE},
                             {"false", TOKEN_FALSE},

                             {"mut", TOKEN_MUT},
                             {"if", TOKEN_IF},
                             {"ef", TOKEN_EF},
                             {"else", TOKEN_ELSE},
                             {"while", TOKEN_WHILE},
                             {"for", TOKEN_FOR},
                             {"in", TOKEN_IN},
                             {"switch", TOKEN_SWITCH},
                             {"match", TOKEN_MATCH},
                             {"fn", TOKEN_FN},
                             {"return", TOKEN_RETURN},
                             {"use", TOKEN_USE},
                             {"from", TOKEN_FROM},
                             {"module", TOKEN_MODULE},
                             {"extern", TOKEN_EXTERN},
                             {"export", TOKEN_EXPORT},
                             {"static", TOKEN_STATIC},
                             {"comptime", TOKEN_COMPTIME},

                             // --- FULL OOP SUITE ---
                             {"class", TOKEN_CLASS},
                             {"extends", TOKEN_EXTENDS},
                             {"impl", TOKEN_IMPL},
                             {"trait", TOKEN_TRAIT},
                             {"virtual", TOKEN_VIRTUAL},
                             {"override", TOKEN_OVERRIDE},
                             {"super", TOKEN_SUPER},
                             {"self", TOKEN_SELF},
                             {"dyn", TOKEN_DYN},
                             {"pub", TOKEN_PUB},
                             {"pb", TOKEN_PUB},
                             {"pri", TOKEN_PRI},
                             {"init", TOKEN_INIT},
                             {"deinit", TOKEN_DEINIT},

                             {NULL, 0}};

/* =========================================================
   SAFETY HELPERS
   ========================================================= */

static inline int at_end(Lexer *l) { return *l->current == '\0'; }

static inline char peek(Lexer *l) { return *l->current; }

static inline char peek_next(Lexer *l) {
  if (at_end(l))
    return '\0';
  return l->current[1];
}

static inline char advance(Lexer *l) {
  char c = *l->current++;
  l->column++;
  return c;
}

static int match(Lexer *l, char expected) {
  if (at_end(l) || *l->current != expected)
    return 0;
  l->current++;
  l->column++;
  return 1;
}

/* =========================================================
   TOKEN CREATION
   ========================================================= */

static Token make_token(Lexer *l, TokenType type, int start_col) {
  Token t;
  t.type = type;
  t.start = l->start;
  t.length = (size_t)(l->current - l->start);
  t.line = l->line;
  t.column = start_col;
  return t;
}

static Token error_token(Lexer *l, const char *msg) {
  Token t;
  t.type = TOKEN_ERROR;
  t.start = msg;
  t.length = (size_t)strlen(msg);
  t.line = l->line;
  t.column = l->column;
  return t;
}

/* =========================================================
   TRIVIA SKIP
   ========================================================= */

static void skip_trivia(Lexer *l) {
  for (;;) {
    char c = peek(l);
    switch (c) {
    case ' ':
    case '\t':
    case '\r':
      advance(l);
      break;
    case '/':
      if (peek_next(l) == '/') {
        while (!at_end(l) && peek(l) != '\n')
          advance(l);
      } else if (peek_next(l) == '*') {
        advance(l);
        advance(l); // Consume /*
        int depth = 1;
        while (depth > 0 && !at_end(l)) {
          if (peek(l) == '/' && peek_next(l) == '*') {
            advance(l);
            advance(l);
            depth++;
          } else if (peek(l) == '*' && peek_next(l) == '/') {
            advance(l);
            advance(l);
            depth--;
          } else {
            if (peek(l) == '\n') {
              l->line++;
              l->column = 1;
            }
            advance(l);
          }
        }
      } else
        return;
      break;
    default:
      return;
    }
  }
}

/* =========================================================
   SCANNERS
   ========================================================= */

static Token scan_identifier(Lexer *l, int start_col) {
  while (isalnum((unsigned char)peek(l)) || peek(l) == '_')
    advance(l);

  int len = (int)(l->current - l->start);
  for (int i = 0; keywords[i].text; i++) {
    if ((int)strlen(keywords[i].text) == len &&
        strncmp(l->start, keywords[i].text, len) == 0) {
      return make_token(l, keywords[i].type, start_col);
    }
  }
  return make_token(l, TOKEN_IDENTIFIER, start_col);
}

static Token scan_number(Lexer *l, int start_col) {
  // Binary literal support: 0b...
  if (*l->start == '0' && (peek(l) == 'b' || peek(l) == 'B')) {
    advance(l);
    while (peek(l) == '0' || peek(l) == '1')
      advance(l);
    return make_token(l, TOKEN_VARDATA, start_col);
  }

  while (isdigit((unsigned char)peek(l)))
    advance(l);

  // Fractional part
  if (peek(l) == '.' && isdigit((unsigned char)peek_next(l))) {
    advance(l);
    while (isdigit((unsigned char)peek(l)))
      advance(l);
  }
  return make_token(l, TOKEN_VARDATA, start_col);
}

static Token scan_char(Lexer *l, int start_col) {
  while (!at_end(l) && peek(l) != '\'') {
    if (peek(l) == '\\')
      advance(l);
    advance(l);
  }
  if (at_end(l))
    return error_token(l, "Unterminated character literal.");
  advance(l);
  return make_token(l, TOKEN_CHAR, start_col);
}

static Token scan_string_all(Lexer *l, char quote_type, int is_raw,
                             int is_interp, int start_col) {
  int is_triple = 0;
  if (quote_type == '"' && peek(l) == '"' && peek_next(l) == '"') {
    is_triple = 1;
    advance(l);
    advance(l);
    advance(l);
  }

  while (!at_end(l)) {
    if (is_triple) {
      if (peek(l) == '"' && peek_next(l) == '"' && l->current[2] == '"') {
        advance(l);
        advance(l);
        advance(l);
        goto success;
      }
    } else if (peek(l) == quote_type) {
      advance(l);
      goto success;
    }

    if (peek(l) == '\\' && !is_raw) {
      advance(l);
      if (!at_end(l))
        advance(l);
    } else {
      if (peek(l) == '\n') {
        l->line++;
        l->column = 1;
      }
      advance(l);
    }
  }
  return error_token(l, "Unterminated string literal.");

success:
  return make_token(l, is_interp ? TOKEN_STRING_INTERP : TOKEN_VARDATA,
                    start_col);
}

/* =========================================================
   SYMBOLS & MAIN FSM
   ========================================================= */
static Token symbol(Lexer *l, char c, int start_col) {
  switch (c) {
  case '{':
    return make_token(l, TOKEN_LBRACE, start_col);
  case '}':
    return make_token(l, TOKEN_RBRACE, start_col);
  case '[':
    return make_token(l, TOKEN_LBRACKET, start_col);
  case ']':
    return make_token(l, TOKEN_RBRACKET, start_col);
  case '(':
    return make_token(l, TOKEN_LPAREN, start_col);
  case ')':
    return make_token(l, TOKEN_RPAREN, start_col);
  case ',':
    return make_token(l, TOKEN_COMMA, start_col);
  case ';':
    return make_token(l, TOKEN_SEMICOLON, start_col);
  case '?':
    return make_token(l, TOKEN_QUESTION, start_col);
  case '@':
    return make_token(l, TOKEN_AT, start_col);
  case '~':
    return make_token(l, TOKEN_TILDE, start_col);
  case '^':
    return make_token(l, TOKEN_CARET, start_col);

  case '%':
    if (match(l, '='))
      return make_token(l, TOKEN_PERCENT_EQUAL, start_col);
    return make_token(l, TOKEN_PERCENT, start_col);

  case ':':
    if (match(l, ':'))
      return make_token(l, TOKEN_COLON_COLON, start_col);
    return make_token(l, TOKEN_COLON, start_col);

  case '-':
    if (match(l, '>'))
      return make_token(l, TOKEN_THIN_ARROW, start_col);
    if (match(l, '='))
      return make_token(l, TOKEN_MINUS_EQUAL, start_col);
    return make_token(l, TOKEN_MINUS, start_col);

  case '+':
    if (match(l, '='))
      return make_token(l, TOKEN_PLUS_EQUAL, start_col);
    return make_token(l, TOKEN_PLUS, start_col);

  case '*':
    if (match(l, '='))
      return make_token(l, TOKEN_STAR_EQUAL, start_col);
    return make_token(l, TOKEN_STAR, start_col);

  case '/':
    if (match(l, '='))
      return make_token(l, TOKEN_SLASH_EQUAL, start_col);
    return make_token(l, TOKEN_SLASH, start_col);

  case '=':
    if (match(l, '>'))
      return make_token(l, TOKEN_ARROW, start_col);
    return make_token(l, match(l, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL,
                      start_col);

  case '!':
    return make_token(l, match(l, '=') ? TOKEN_BANG_EQUAL : TOKEN_BANG,
                      start_col);

  case '<':
    if (match(l, '<'))
      return make_token(l, TOKEN_LEFT_SHIFT, start_col);
    return make_token(l, match(l, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS,
                      start_col);

  case '>':
    if (match(l, '>'))
      return make_token(l, TOKEN_RIGHT_SHIFT, start_col);
    return make_token(l, match(l, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER,
                      start_col);

  case '&':
    if (match(l, '&'))
      return make_token(l, TOKEN_AND_AND, start_col);
    if (peek(l) == '"') {
      int quote_col = l->column;
      advance(l);
      return scan_string_all(l, '"', 0, 1, quote_col);
    }
    return make_token(l, TOKEN_AMP, start_col);

  case '|':
    if (match(l, '|'))
      return make_token(l, TOKEN_OR_OR, start_col);
    return make_token(l, TOKEN_PIPE, start_col);

  case '.':
    if (peek(l) == '.') {
      advance(l);
      if (peek(l) == '.') {
        advance(l);
        return make_token(l, TOKEN_DOT_DOT_DOT, start_col);
      }
      return make_token(l, TOKEN_DOT_DOT, start_col);
    }
    return make_token(l, TOKEN_DOT, start_col);

  default: {
    static char err_buf[32];
    snprintf(err_buf, sizeof(err_buf), "Unexpected char: '%c'", c);
    return error_token(l, err_buf);
  }
  }
}

Token next_token(Lexer *l) {
  skip_trivia(l);
  l->start = l->current;
  int start_col = l->column;

  if (at_end(l)) {
    return (Token){TOKEN_EOF, l->current, 0, l->line, l->column};
  }

  if (peek(l) == '\n') {
    int start_line = l->line;

    while (peek(l) == '\n') {
      advance(l);
      l->line++;
      l->column = 1;

      skip_trivia(l);
      if (peek(l) != '\n')
        break;
    }

    return (Token){TOKEN_NEWLINE, l->start, (size_t)(l->current - l->start),
                   start_line, start_col};
  }

  char c = advance(l);

  if (c == '\'')
    return scan_char(l, start_col);
  if (c == '`')
    return scan_string_all(l, '`', 0, 0, start_col);
  if (c == 'r' && peek(l) == '"') {
    advance(l);
    return scan_string_all(l, '"', 1, 0, start_col);
  }
  if (c == '"')
    return scan_string_all(l, '"', 0, 0, start_col);

  if (isalpha((unsigned char)c) || c == '_')
    return scan_identifier(l, start_col);
  if (isdigit((unsigned char)c))
    return scan_number(l, start_col);

  return symbol(l, c, start_col);
}

/* =========================================================
   INIT & DEBUG
   ========================================================= */

void lexer_init(Lexer *l, const char *src) {
  l->start = src;
  l->current = src;
  l->line = 1;
  l->column = 1;
}

const char *token_type_to_string(TokenType t) {
  switch (t) {
  case TOKEN_PLUS:
    return "PLUS";
  case TOKEN_MINUS:
    return "MINUS";
  case TOKEN_STAR:
    return "STAR";
  case TOKEN_SLASH:
    return "SLASH";
  case TOKEN_IDENTIFIER:
    return "IDENTIFIER";
  case TOKEN_VARDATA:
    return "VARDATA";
  case TOKEN_CHAR:
    return "CHAR";
  case TOKEN_STRING_INTERP:
    return "STRING_INTERP";
  case TOKEN_MUT:
    return "MUT";
  case TOKEN_IF:
    return "IF";
  case TOKEN_EF:
    return "EF";
  case TOKEN_ELSE:
    return "ELSE";
  case TOKEN_WHILE:
    return "WHILE";
  case TOKEN_FOR:
    return "FOR";
  case TOKEN_IN:
    return "IN";
  case TOKEN_MATCH:
    return "MATCH";
  case TOKEN_SWITCH:
    return "SWITCH";
  case TOKEN_FN:
    return "FN";
  case TOKEN_RETURN:
    return "RETURN";
  case TOKEN_USE:
    return "USE";
  case TOKEN_FROM:
    return "FROM";
  case TOKEN_MODULE:
    return "MODULE";
  case TOKEN_EXTERN:
    return "EXTERN";
  case TOKEN_EXPORT:
    return "EXPORT";
  case TOKEN_STATIC:
    return "STATIC";
  case TOKEN_COMPTIME:
    return "COMPTIME";
  case TOKEN_CLASS:
    return "CLASS";
  case TOKEN_EXTENDS:
    return "EXTENDS";
  case TOKEN_IMPL:
    return "IMPL";
  case TOKEN_TRAIT:
    return "TRAIT";
  case TOKEN_VIRTUAL:
    return "VIRTUAL";
  case TOKEN_OVERRIDE:
    return "OVERRIDE";
  case TOKEN_SUPER:
    return "SUPER";
  case TOKEN_SELF:
    return "SELF";
  case TOKEN_DYN:
    return "DYN";
  case TOKEN_PUB:
    return "PUB";
  case TOKEN_PRI:
    return "PRI";
  case TOKEN_INIT:
    return "INIT";
  case TOKEN_DEINIT:
    return "DEINIT";
  case TOKEN_LBRACE:
    return "LBRACE";
  case TOKEN_RBRACE:
    return "RBRACE";
  case TOKEN_LBRACKET:
    return "LBRACKET";
  case TOKEN_RBRACKET:
    return "RBRACKET";
  case TOKEN_LPAREN:
    return "LPAREN";
  case TOKEN_RPAREN:
    return "RPAREN";
  case TOKEN_COMMA:
    return "COMMA";
  case TOKEN_COLON:
    return "COLON";
  case TOKEN_COLON_COLON:
    return "COLON_COLON";
  case TOKEN_SEMICOLON:
    return "SEMICOLON";
  case TOKEN_NEWLINE:
    return "NEWLINE";
  case TOKEN_QUESTION:
    return "QUESTION";
  case TOKEN_AT:
    return "AT";
  case TOKEN_EQUAL:
    return "EQUAL";
  case TOKEN_EQUAL_EQUAL:
    return "EQUAL_EQUAL";
  case TOKEN_BANG:
    return "BANG";
  case TOKEN_BANG_EQUAL:
    return "BANG_EQUAL";
  case TOKEN_LESS:
    return "LESS";
  case TOKEN_LESS_EQUAL:
    return "LESS_EQUAL";
  case TOKEN_GREATER:
    return "GREATER";
  case TOKEN_GREATER_EQUAL:
    return "GREATER_EQUAL";
  case TOKEN_LEFT_SHIFT:
    return "LEFT_SHIFT";
  case TOKEN_RIGHT_SHIFT:
    return "RIGHT_SHIFT";
  case TOKEN_AMP:
    return "AMP";
  case TOKEN_PIPE:
    return "PIPE";
  case TOKEN_CARET:
    return "CARET";
  case TOKEN_TILDE:
    return "TILDE";
  case TOKEN_AND_AND:
    return "AND_AND";
  case TOKEN_OR_OR:
    return "OR_OR";
  case TOKEN_ARROW:
    return "ARROW";
  case TOKEN_THIN_ARROW:
    return "THIN_ARROW";
  case TOKEN_DOT_DOT_DOT:
    return "DOT_DOT_DOT";
  case TOKEN_DOT:
    return "DOT";
  case TOKEN_EOF:
    return "EOF";
  case TOKEN_ERROR:
    return "ERROR";
  case TOKEN_MINUS_EQUAL:
    return "MINUS_EQUAL";
  case TOKEN_STAR_EQUAL:
    return "STAR_EQUAL";
  case TOKEN_SLASH_EQUAL:
    return "SLASH_EQUAL";
  case TOKEN_PERCENT_EQUAL:
    return "PERCENT_EQUAL";
  case TOKEN_PLUS_EQUAL:
    return "PLUS_EQUAL";
  case TOKEN_PERCENT:
    return "PERCENT";
  case TOKEN_DOT_DOT:
    return "DOT_DOT";
  default:
    return "UNKNOWN";
  }
}

void print_token(Token t) {
  printf("%-15s | '%.*s' | (%d:%d)\n", token_type_to_string(t.type),
         (int)t.length, t.start, t.line, t.column);
}
