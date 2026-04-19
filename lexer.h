#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>

/* =========================================================
   TOKEN DEFINITIONS
   ========================================================= */

typedef enum {
  // Literals & Identifiers
  TOKEN_IDENTIFIER,
  TOKEN_VARDATA,       // Unified literal: int / float / string / bigint / raw /
                       // binary
  TOKEN_CHAR,          // Single character literal 'a'
  TOKEN_STRING_INTERP, // Interpolated string &"..."
  TOKEN_TRUE,          // true literal
  TOKEN_FALSE,         // false literal
  TOKEN_NEN,           // nen literal (null)

  // Brackets & Parentheses
  TOKEN_LBRACKET, // [
  TOKEN_RBRACKET, // ]
  TOKEN_LPAREN,   // (
  TOKEN_RPAREN,   // )
  TOKEN_LBRACE,   // {
  TOKEN_RBRACE,   // }

  // Punctuation & Scoping
  TOKEN_COMMA,       // ,
  TOKEN_DOT,         // .  <-- Added for member access
  TOKEN_DOT_DOT,     // .. (Exclusive range)
  TOKEN_DOT_DOT_DOT, // ... (Inclusive range)
  TOKEN_COLON,       // :
  TOKEN_COLON_COLON, // :: (Path separator / scope)
  TOKEN_SEMICOLON,   // ;
  TOKEN_ARROW,       // => (Match arm / Thick arrow)
  TOKEN_THIN_ARROW,  // -> (Return type / Function arrow)
  TOKEN_QUESTION,    // ? (Ternary)
  TOKEN_AT,          // @ (Attribute / Decorator)

  // Assignment & Comparison
  TOKEN_EQUAL,         // =
  TOKEN_EQUAL_EQUAL,   // ==
  TOKEN_PLUS_EQUAL,    // +=
  TOKEN_MINUS_EQUAL,   // -=
  TOKEN_STAR_EQUAL,    // *=
  TOKEN_SLASH_EQUAL,   // /=
  TOKEN_PERCENT_EQUAL, // %=
  TOKEN_BANG,          // !
  TOKEN_BANG_EQUAL,    // !=
  TOKEN_LESS,          // <
  TOKEN_LESS_EQUAL,    // <=
  TOKEN_GREATER,       // >
  TOKEN_GREATER_EQUAL, // >=

  // Arithmetic
  TOKEN_PLUS,    // +
  TOKEN_MINUS,   // -
  TOKEN_STAR,    // *
  TOKEN_SLASH,   // /
  TOKEN_PERCENT, // % (Modulo)

  // Bitwise Operators
  TOKEN_AMP,         // &
  TOKEN_PIPE,        // |
  TOKEN_CARET,       // ^
  TOKEN_TILDE,       // ~
  TOKEN_LEFT_SHIFT,  // <<
  TOKEN_RIGHT_SHIFT, // >>

  // Logical Operators
  TOKEN_AND_AND, // &&
  TOKEN_OR_OR,   // ||

  // Keywords (Control Flow)
  TOKEN_IF,
  TOKEN_EF,
  TOKEN_ELSE,
  TOKEN_WHILE,
  TOKEN_FOR,
  TOKEN_IN,
  TOKEN_MATCH,
  TOKEN_SWITCH,

  // Keywords (Functions & Declarations)
  TOKEN_FN,       // fn
  TOKEN_RETURN,   // return
  TOKEN_MUT,      // mut
  TOKEN_STATIC,   // static
  TOKEN_COMPTIME, // comptime

  // Keywords (Modules & FFI)
  TOKEN_USE,    // use
  TOKEN_FROM,   // from
  TOKEN_MODULE, // module
  TOKEN_EXTERN, // extern
  TOKEN_EXPORT, // export

  // Keywords (Full OOP Suite)
  TOKEN_CLASS,    // class
  TOKEN_EXTENDS,  // extends
  TOKEN_IMPL,     // impl
  TOKEN_TRAIT,    // trait
  TOKEN_VIRTUAL,  // virtual
  TOKEN_OVERRIDE, // override
  TOKEN_SUPER,    // super
  TOKEN_SELF,     // self
  TOKEN_DYN,      // dyn
  TOKEN_PUB,      // pub (Public) - Also matches 'pb' alias
  TOKEN_PRI,      // pri (Private)
  TOKEN_INIT,     // init (Constructor)
  TOKEN_DEINIT,   // deinit (Destructor)

  // Virtual
  TOKEN_EOF,  // End of file
  TOKEN_ERROR // Lexing error
} TokenType;

/* =========================================================
   TOKEN STRUCT
   ========================================================= */

typedef struct {
  TokenType type;

  const char *start; // Pointer to start of token in source
  size_t length;     // Length of token text

  int line;   // 1-based line number
  int column; // 0-based column number
} Token;

/* =========================================================
   LEXER STATE
   ========================================================= */

typedef struct {
  const char *start;   // Start of current token being scanned
  const char *current; // Next character to be processed

  int line;   // Current line tracker
  int column; // Current column tracker
} Lexer;

/* =========================================================
   API
   ========================================================= */

/**
 * Initializes the lexer state with the provided source string.
 */
void lexer_init(Lexer *l, const char *source);

/**
 * Scans the source and returns the next valid Token.
 */
Token next_token(Lexer *l);

/**
 * Converts a TokenType enum value to a human-readable string.
 */
const char *token_type_to_string(TokenType t);

/**
 * Debug helper to print token information.
 */
void print_token(Token t);

#endif