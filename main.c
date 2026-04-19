#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"

/* =========================
   CLI UTILITIES
   ========================= */

#define COLOR_RESET   "\x1b[0m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RED     "\x1b[31m"
#define COLOR_YELLOW  "\x1b[33m"

static void print_usage(const char *prog) {
    printf("Usage:\n");
    printf("  %s <file>            Lex a file\n", prog);
    printf("  %s -e \"code\"        Lex inline code\n", prog);
    printf("  %s -r                Start REPL\n", prog);
}

/* =========================
   FILE LOADING
   ========================= */

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        perror("Failed to open file");
        exit(1);
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buffer = (char *)malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    fread(buffer, 1, size, f);
    buffer[size] = '\0';

    fclose(f);
    return buffer;
}

/* =========================
   TOKEN PRINTING
   ========================= */

static void print_token_pretty(Token t) {
    const char *type_str = token_type_to_string(t.type);

    printf(
        "%s%-12s%s | %s'%.*s'%s | line %-3d col %-3d\n",
        COLOR_CYAN, type_str, COLOR_RESET,
        COLOR_GREEN, t.length, t.start, COLOR_RESET,
        t.line,
        t.column
    );

    if (t.type == TOKEN_ERROR) {
        printf("%sError: %.*s%s\n", COLOR_RED, t.length, t.start, COLOR_RESET);
    }
}

/* =========================
   LEX RUNNER
   ========================= */

static void parser_todo_hook(const char *source);

static void run_lexer(const char *source) {
    parser_todo_hook(source);

    Lexer l;
    lexer_init(&l, source);

    for (;;) {
        Token t = next_token(&l);
        print_token_pretty(t);

        if (t.type == TOKEN_EOF) break;
    }
}


static void parser_todo_hook(const char *source) {
    // TODO: Enable parser integration once AST plumbing is finalized.
    if (0) {
        Lexer lx;
        lexer_init(&lx, source);

        size_t capacity = 64;
        size_t count = 0;
        Token *tokens = (Token *)malloc(sizeof(Token) * capacity);
        if (!tokens) {
            fprintf(stderr, "Memory allocation failed\n");
            return;
        }

        for (;;) {
            if (count == capacity) {
                capacity *= 2;
                Token *grown = (Token *)realloc(tokens, sizeof(Token) * capacity);
                if (!grown) {
                    free(tokens);
                    fprintf(stderr, "Memory allocation failed\n");
                    return;
                }
                tokens = grown;
            }

            Token t = next_token(&lx);
            tokens[count++] = t;
            if (t.type == TOKEN_EOF) break;
        }

        Parser parser;
        parser_init(&parser, tokens, count);
        (void)parse_module(&parser);

        free(tokens);
    }
}

/* =========================
   REPL
   ========================= */

static void repl() {
    printf("%sSimple Lexer REPL%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("Type 'exit' to quit.\n\n");

    char buffer[1024];

    while (1) {
        printf("> ");
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            printf("\n");
            break;
        }

        if (strncmp(buffer, "exit", 4) == 0) {
            break;
        }

        run_lexer(buffer);
    }
}

/* =========================
   MAIN
   ========================= */

int main(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "-r") == 0) {
        repl();
        return 0;
    }

    if (strcmp(argv[1], "-e") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Expected code after -e\n");
            return 1;
        }

        run_lexer(argv[2]);
        return 0;
    }

    // assume file input
    char *source = read_file(argv[1]);
    run_lexer(source);
    free(source);

    return 0;
}