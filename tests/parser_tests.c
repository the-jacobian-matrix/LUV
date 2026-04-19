#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "../lexer.h"
#include "../parser.h"

static Token *tokenize(const char *src, size_t *out_count) {
  Lexer l;
  lexer_init(&l, src);

  Token *tokens = NULL;
  size_t count = 0;
  for (;;) {
    Token t = next_token(&l);
    tokens = (Token *)realloc(tokens, (count + 1) * sizeof(Token));
    tokens[count++] = t;
    if (t.type == TOKEN_EOF)
      break;
  }

  *out_count = count;
  return tokens;
}

static void test_shorthand_function() {
  size_t count;
  Token *tokens = tokenize("sum(a, b) -> a", &count);
  Parser p;
  parser_init(&p, tokens, count);

  AstNode *n = parse_declaration(&p);
  assert(n->kind == AST_FUNCTION_DECL);
  assert(n->as.fn_decl.param_count == 2);
  assert(n->as.fn_decl.body->kind == AST_BLOCK);
  assert(n->as.fn_decl.body->as.block.count == 1);
  assert(n->as.fn_decl.body->as.block.statements[0]->kind == AST_RETURN_STMT);

  free(tokens);
}

static void test_call_expression() {
  size_t count;
  Token *tokens = tokenize("foo(a, b)", &count);
  Parser p;
  parser_init(&p, tokens, count);

  AstNode *n = parse_declaration(&p);
  assert(n->kind == AST_CALL_EXPR);
  assert(n->as.call.arg_count == 2);

  free(tokens);
}

int main(void) {
  test_shorthand_function();
  test_call_expression();
  printf("parser tests passed\n");
  return 0;
}
