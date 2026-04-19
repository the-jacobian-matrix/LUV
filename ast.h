#ifndef LUV_AST_H
#define LUV_AST_H

#include <stdbool.h>
#include <stddef.h>

typedef struct ASTNode ASTNode;

typedef enum NodeType {
    /* Top-level declarations */
    NODE_MODULE,
    NODE_IMPORT,
    NODE_FUNCTION,
    NODE_CLASS,
    NODE_IMPL,
    NODE_PARAM,
    NODE_BLOCK,
    NODE_VAR_DECL,

    /* Statements / control flow */
    NODE_IF,
    NODE_EF,
    NODE_ELSE,
    NODE_WHILE,
    NODE_FOR,
    NODE_MATCH,
    NODE_RETURN,

    /* Expressions */
    NODE_BINARY,
    NODE_UNARY,
    NODE_CALL,
    NODE_MEMBER,
    NODE_INDEX,
    NODE_ASSIGN,

    /* Primaries */
    NODE_IDENTIFIER,
    NODE_LITERAL,
    NODE_STRING_INTERP,
    NODE_RANGE
} NodeType;

typedef struct ASTNodeList {
    ASTNode **items;
    size_t len;
    size_t cap;
} ASTNodeList;

typedef struct ParamList {
    ASTNode **items;
    size_t len;
    size_t cap;
} ParamList;

typedef struct MatchArm {
    ASTNode *pattern;
    ASTNode *guard;   /* optional */
    ASTNode *value;
} MatchArm;

typedef struct MatchArmList {
    MatchArm *items;
    size_t len;
    size_t cap;
} MatchArmList;

typedef struct ImportPathList {
    ASTNode **items;
    size_t len;
    size_t cap;
} ImportPathList;

typedef enum LiteralKind {
    LITERAL_NUMBER,
    LITERAL_STRING,
    LITERAL_CHAR,
    LITERAL_BOOL,
    LITERAL_NEN
} LiteralKind;

typedef struct LiteralPayload {
    const char *raw_start; /* raw token span start */
    size_t raw_len;        /* raw token span length */
    LiteralKind kind;      /* decoded literal kind */
} LiteralPayload;

typedef enum StringInterpSegmentKind {
    STRING_SEG_LITERAL,
    STRING_SEG_EXPR
} StringInterpSegmentKind;

typedef struct StringInterpSegment {
    StringInterpSegmentKind kind;
    union {
        struct {
            const char *text_start;
            size_t text_len;
        } literal;
        ASTNode *expr;
    } as;
} StringInterpSegment;

typedef struct StringInterpSegmentList {
    StringInterpSegment *items;
    size_t len;
    size_t cap;
} StringInterpSegmentList;

struct ASTNode {
    NodeType type;
    int line;
    int column;

    union {
        struct {
            ASTNodeList items;
        } module;

        struct {
            ImportPathList path;
            const char *alias; /* optional */
        } import_decl;

        struct {
            bool has_fn_keyword;     /* stylistic/diagnostic tracking */
            bool is_pub;
            bool is_extern;
            bool is_static;
            bool is_comptime;

            ASTNode *name;           /* identifier */
            ParamList params;
            ASTNode *return_type;    /* optional */

            ASTNode *body_block;     /* optional, block-bodied form */
            ASTNode *body_expr;      /* optional, one-line expression form */
        } function;

        struct {
            ASTNode *name;
            ASTNodeList members;
        } class_decl;

        struct {
            ASTNode *target_type;
            ASTNodeList members;
        } impl_decl;

        struct {
            ASTNode *name;
            ASTNode *type_expr;      /* optional */
            ASTNode *default_value;  /* optional */
            bool is_mut;
            bool is_dyn;
        } param;

        struct {
            ASTNodeList statements;
        } block;

        struct {
            ASTNode *name;
            ASTNode *type_expr;      /* optional */
            ASTNode *init;           /* optional */
            bool is_mut;
            bool is_dyn;
        } var_decl;

        struct {
            ASTNode *cond;
            ASTNode *then_branch;
            ASTNode *ef_branch;      /* optional */
            ASTNode *else_branch;    /* optional */
        } if_stmt;

        struct {
            ASTNode *cond;
            ASTNode *body;
        } ef_stmt;

        struct {
            ASTNode *body;
        } else_stmt;

        struct {
            ASTNode *cond;
            ASTNode *body;
        } while_stmt;

        struct {
            ASTNode *iter_var;
            ASTNode *iterable;
            ASTNode *body;
        } for_stmt;

        struct {
            ASTNode *value;
            MatchArmList arms;
        } match_stmt;

        struct {
            ASTNode *value; /* optional */
        } return_stmt;

        struct {
            ASTNode *left;
            ASTNode *right;
            const char *op;
        } binary;

        struct {
            ASTNode *operand;
            const char *op;
        } unary;

        struct {
            ASTNode *callee;
            ASTNodeList args;
        } call;

        struct {
            ASTNode *object;
            ASTNode *member;
        } member;

        struct {
            ASTNode *object;
            ASTNode *index;
        } index;

        struct {
            ASTNode *target;
            ASTNode *value;
            const char *op; /* =, +=, etc. */
        } assign;

        struct {
            const char *name;
            size_t name_len;
        } identifier;

        LiteralPayload literal;

        struct {
            StringInterpSegmentList segments;
        } string_interp;

        struct {
            ASTNode *start;
            ASTNode *end;
            bool inclusive;
        } range;
    } as;
};

#endif /* LUV_AST_H */
