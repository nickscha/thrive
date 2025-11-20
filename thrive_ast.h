#ifndef THRIVE_AST_H
#define THRIVE_AST_H

#include "thrive.h"
#include "thrive_tokenizer.h"

typedef enum thrive_ast_type
{
    AST_INT,
    AST_FLOAT,
    AST_VAR,
    AST_ADD,
    AST_SUB,
    AST_MUL,
    AST_DIV,
    AST_ASSIGN,
    AST_DECL, /* u32 x = expr */
    AST_RETURN

} thrive_ast_type;

typedef struct thrive_ast
{
    thrive_ast_type type;

    union
    {
        i32 int_val;   /* AST_INT */
        f64 float_val; /* AST_FLOAT */
        u8 name[32];   /* AST_VAR or DECL varname */

        struct
        { /* binary ops / assignment */
            u16 left;
            u16 right;
        } op;

        struct
        { /* DECL */
            u8 name[32];
            u16 expr;
        } decl;

        u16 expr; /* return */
    } v;

} thrive_ast;

typedef struct thrive_parser
{
    thrive_token *toks;
    u32 count;
    u32 pos;

    thrive_ast *ast;
    u32 ast_cap;
    u32 ast_size;
} thrive_parser;

THRIVE_API THRIVE_INLINE thrive_token *peek(thrive_parser *p)
{
    return (p->pos < p->count) ? &p->toks[p->pos] : &p->toks[p->count - 1];
}

THRIVE_API THRIVE_INLINE thrive_token *next(thrive_parser *p)
{
    if (p->pos < p->count)
        p->pos++;
    return peek(p);
}

THRIVE_API THRIVE_INLINE i32 accept(thrive_parser *p, thrive_token_type t)
{
    if (peek(p)->type == t)
    {
        next(p);
        return 1;
    }
    return 0;
}

THRIVE_API THRIVE_INLINE u16 ast_new(thrive_parser *p)
{
    u16 id = (u16)p->ast_size;
    p->ast_size++;
    return id;
}

THRIVE_API i32 precedence(thrive_token_type t)
{
    switch (t)
    {
    case THRIVE_TOKEN_MUL:
    case THRIVE_TOKEN_DIV:
        return 50;

    case THRIVE_TOKEN_ADD:
    case THRIVE_TOKEN_SUB:
        return 40;

    case THRIVE_TOKEN_ASSIGN:
        return 10; /* right-associative */

    default:
        return -1;
    }
}

THRIVE_API u16 parse_expr_bp(thrive_parser *p, i32 min_bp);

THRIVE_API u16 parse_primary(thrive_parser *p)
{
    thrive_token *t = peek(p);

    /* integer literal */
    if (t->type == THRIVE_TOKEN_INTEGER)
    {
        u16 id = ast_new(p);
        p->ast[id].type = AST_INT;
        p->ast[id].v.int_val = t->value.integer;
        next(p);
        return id;
    }

    /* float literal */
    if (t->type == THRIVE_TOKEN_FLOAT)
    {
        u16 id = ast_new(p);
        p->ast[id].type = AST_FLOAT;
        p->ast[id].v.float_val = t->value.floating;
        next(p);
        return id;
    }

    /* identifier */
    if (t->type == THRIVE_TOKEN_VAR)
    {
        u16 id = ast_new(p);
        p->ast[id].type = AST_VAR;

        /* copy name */
        {
            i32 i = 0;
            while (t->value.name[i])
            {
                p->ast[id].v.name[i] = t->value.name[i];
                i++;
            }
            p->ast[id].v.name[i] = 0;
        }

        next(p);
        return id;
    }

    /* parenthesized expression */
    if (accept(p, THRIVE_TOKEN_LPAREN))
    {
        u16 inner = parse_expr_bp(p, 0);
        accept(p, THRIVE_TOKEN_RPAREN); /* assume correct syntax */
        return inner;
    }

    /* unreachable for your language */
    return 0;
}

/* ---- Pratt expression parser ---- */

THRIVE_API u16 parse_expr_bp(thrive_parser *p, i32 min_bp)
{
    /* parse primary first */
    u16 left = parse_primary(p);

    for (;;)
    {
        thrive_token_type op = peek(p)->type;
        i32 bp = precedence(op);
        i32 next_min_bp;
        u16 right;
        u16 id;

        if (bp < min_bp)
        {
            break;
        }

        /* determine left & right associativity */
        next_min_bp = bp + (op == THRIVE_TOKEN_ASSIGN ? 0 : 1);

        next(p); /* consume operator */

        right = parse_expr_bp(p, next_min_bp);

        /* build AST node */
        id = ast_new(p);

        switch (op)
        {
        case THRIVE_TOKEN_ADD:
            p->ast[id].type = AST_ADD;
            break;
        case THRIVE_TOKEN_SUB:
            p->ast[id].type = AST_SUB;
            break;
        case THRIVE_TOKEN_MUL:
            p->ast[id].type = AST_MUL;
            break;
        case THRIVE_TOKEN_DIV:
            p->ast[id].type = AST_DIV;
            break;
        case THRIVE_TOKEN_ASSIGN:
            p->ast[id].type = AST_ASSIGN;
            break;
        default:
            break;
        }

        p->ast[id].v.op.left = left;
        p->ast[id].v.op.right = right;

        left = id;
    }

    return left;
}

/* ---- Parse a statement ---- */

THRIVE_API u16 parse_stmt(thrive_parser *p)
{
    peek(p);

    /* u32 var = expr */
    if (accept(p, THRIVE_TOKEN_KEYWORD_U32))
    {
        thrive_token *v = peek(p);

        /* allocate node */
        u16 id = ast_new(p);

        i32 i = 0;

        p->ast[id].type = AST_DECL;

        /* copy var name */
        while (v->value.name[i])
        {
            p->ast[id].v.decl.name[i] = v->value.name[i];
            i++;
        }
        p->ast[id].v.decl.name[i] = 0;

        next(p); /* consume var */

        accept(p, THRIVE_TOKEN_ASSIGN);

        p->ast[id].v.decl.expr = parse_expr_bp(p, 0);

        return id;
    }

    /* ret expr */
    if (accept(p, THRIVE_TOKEN_KEYWORD_RET))
    {
        u16 id = ast_new(p);
        p->ast[id].type = AST_RETURN;
        p->ast[id].v.expr = parse_expr_bp(p, 0);
        return id;
    }

    /* otherwise parse bare expression */
    return parse_expr_bp(p, 0);
}

/* ---- Parse a whole program ---- */

THRIVE_API void thrive_parse_program(
    thrive_token *tokens,
    u32 token_count,
    thrive_ast *ast_buf,
    u32 ast_cap,
    u32 *ast_out)
{
    thrive_parser P;
    P.toks = tokens;
    P.count = token_count;
    P.pos = 0;
    P.ast = ast_buf;
    P.ast_cap = ast_cap;
    P.ast_size = 0;

    while (peek(&P)->type != THRIVE_TOKEN_EOF &&
           P.ast_size < P.ast_cap)
    {
        parse_stmt(&P);
    }

    *ast_out = P.ast_size;
}

#endif /* THRIVE_AST_H */
