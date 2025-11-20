/* thrive.h - v0.1 - public domain data structures - nickscha 2025

A C89 standard compliant, nostdlib (no C Standard Library) Low Level Programming Language inbetween Assembly and C (THRIVE).

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
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

    union v
    {
        i32 int_val;   /* AST_INT */
        f64 float_val; /* AST_FLOAT */
        u8 name[32];   /* AST_VAR or DECL varname */

        struct op
        { /* binary ops / assignment */
            u16 left;
            u16 right;
        } op;

        struct decl
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

THRIVE_API THRIVE_INLINE thrive_token *thrive_ast_peek(thrive_parser *p)
{
    return (p->pos < p->count) ? &p->toks[p->pos] : &p->toks[p->count - 1];
}

THRIVE_API THRIVE_INLINE thrive_token *thrive_ast_next(thrive_parser *p)
{
    if (p->pos < p->count)
    {
        p->pos++;
    }

    return thrive_ast_peek(p);
}

THRIVE_API THRIVE_INLINE i32 thrive_ast_accept(thrive_parser *p, thrive_token_type t)
{
    if (thrive_ast_peek(p)->type == t)
    {
        thrive_ast_next(p);
        return 1;
    }

    return 0;
}

THRIVE_API THRIVE_INLINE u16 thrive_ast_create(thrive_parser *p)
{
    u16 id = (u16)p->ast_size;
    p->ast_size++;
    return id;
}

THRIVE_API i32 thrive_ast_precedence(thrive_token_type t)
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

THRIVE_API u16 thrive_ast_parse_expr_bp(thrive_parser *p, i32 min_bp);

THRIVE_API u16 thrive_ast_parse_primary(thrive_parser *p)
{
    thrive_token *t = thrive_ast_peek(p);

    /* integer literal */
    if (t->type == THRIVE_TOKEN_INTEGER)
    {
        u16 id = thrive_ast_create(p);
        p->ast[id].type = AST_INT;
        p->ast[id].v.int_val = t->value.integer;
        thrive_ast_next(p);
        return id;
    }

    /* float literal */
    if (t->type == THRIVE_TOKEN_FLOAT)
    {
        u16 id = thrive_ast_create(p);
        p->ast[id].type = AST_FLOAT;
        p->ast[id].v.float_val = t->value.floating;
        thrive_ast_next(p);
        return id;
    }

    /* identifier */
    if (t->type == THRIVE_TOKEN_VAR)
    {
        u16 id = thrive_ast_create(p);
        p->ast[id].type = AST_VAR;

        /* copy name */
        {
            u32 i = 0;
            u32 n = t->value.view.length;
            u8 *s = t->value.view.start;

            for (i = 0; i < n && i < 31; ++i)
            {
                p->ast[id].v.name[i] = s[i];
            }

            p->ast[id].v.name[i] = 0;
        }

        thrive_ast_next(p);
        return id;
    }

    /* parenthesized expression */
    if (thrive_ast_accept(p, THRIVE_TOKEN_LPAREN))
    {
        u16 inner = thrive_ast_parse_expr_bp(p, 0);
        thrive_ast_accept(p, THRIVE_TOKEN_RPAREN); /* assume correct syntax */
        return inner;
    }

    /* unreachable for your language */
    return 0;
}

THRIVE_API u16 thrive_ast_parse_expr_bp(thrive_parser *p, i32 min_bp)
{
    /* parse primary first */
    u16 left = thrive_ast_parse_primary(p);

    for (;;)
    {
        thrive_token_type op = thrive_ast_peek(p)->type;
        i32 bp = thrive_ast_precedence(op);
        i32 thrive_ast_next_min_bp;
        u16 right;
        u16 id;

        if (bp < min_bp)
        {
            break;
        }

        /* determine left & right associativity */
        thrive_ast_next_min_bp = bp + (op == THRIVE_TOKEN_ASSIGN ? 0 : 1);

        thrive_ast_next(p); /* consume operator */

        right = thrive_ast_parse_expr_bp(p, thrive_ast_next_min_bp);

        /* build AST node */
        id = thrive_ast_create(p);

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

THRIVE_API u16 thrive_ast_parse_statement(thrive_parser *p)
{
    thrive_ast_peek(p);

    /* u32 var = expr */
    if (thrive_ast_accept(p, THRIVE_TOKEN_KEYWORD_U32))
    {
        thrive_token *v = thrive_ast_peek(p);

        /* allocate node */
        u16 id = thrive_ast_create(p);

        p->ast[id].type = AST_DECL;

        /* copy var name */
        {
            u32 i = 0;
            u32 n = v->value.view.length;
            u8 *s = v->value.view.start;

            for (i = 0; i < n && i < 31; ++i)
            {
                p->ast[id].v.decl.name[i] = s[i];
            }

            p->ast[id].v.decl.name[i] = 0;
        }

        thrive_ast_next(p); /* consume var */

        thrive_ast_accept(p, THRIVE_TOKEN_ASSIGN);

        p->ast[id].v.decl.expr = thrive_ast_parse_expr_bp(p, 0);

        return id;
    }

    /* ret expr */
    if (thrive_ast_accept(p, THRIVE_TOKEN_KEYWORD_RET))
    {
        u16 id = thrive_ast_create(p);
        p->ast[id].type = AST_RETURN;
        p->ast[id].v.expr = thrive_ast_parse_expr_bp(p, 0);
        return id;
    }

    /* otherwise parse bare expression */
    return thrive_ast_parse_expr_bp(p, 0);
}

THRIVE_API void thrive_ast_parse(
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

    while (thrive_ast_peek(&P)->type != THRIVE_TOKEN_EOF &&
           P.ast_size < P.ast_cap)
    {
        thrive_ast_parse_statement(&P);
    }

    *ast_out = P.ast_size;
}

#endif /* THRIVE_AST_H */

/*
   ------------------------------------------------------------------------------
   This software is available under 2 licenses -- choose whichever you prefer.
   ------------------------------------------------------------------------------
   ALTERNATIVE A - MIT License
   Copyright (c) 2025 nickscha
   Permission is hereby granted, free of charge, to any person obtaining a copy of
   this software and associated documentation files (the "Software"), to deal in
   the Software without restriction, including without limitation the rights to
   use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
   of the Software, and to permit persons to whom the Software is furnished to do
   so, subject to the following conditions:
   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
   ------------------------------------------------------------------------------
   ALTERNATIVE B - Public Domain (www.unlicense.org)
   This is free and unencumbered software released into the public domain.
   Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
   software, either in source code form or as a compiled binary, for any purpose,
   commercial or non-commercial, and by any means.
   In jurisdictions that recognize copyright laws, the author or authors of this
   software dedicate any and all copyright interest in the software to the public
   domain. We make this dedication for the benefit of the public at large and to
   the detriment of our heirs and successors. We intend this dedication to be an
   overt act of relinquishment in perpetuity of all present and future rights to
   this software under copyright law.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
   ------------------------------------------------------------------------------
*/
