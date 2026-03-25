/* thrive.h - v0.2 - public domain data structures - nickscha 2026

A C89 standard compliant, nostdlib (no C Standard Library)
Low Level Programming Language inbetween Assembly and C (THRIVE).

LANGUAGE SPECIFICATON (WIP)

  General Design (less than C89):
    - procedural
    - no mixed declaration
    - fixed sized types  ; on all platforms
    - no header files    ; just thrive source files
    - no marcos/typedefs
    - no standard library
    - no recursive functions
    - all loop conditions must have a max bound

  Types:
    - s8, s16, s32      ; String type
    - b8, b16, b32, b64 ; Boolean type
    - u8, u16, u32, u64 ; Unsigned Integer type
    - i8, i16, i32, i64 ; Signed Integer type
    - f8, f16, f32, f64 ; Floating type

  Keywords:
    - ret ; Return
    - ext ; External Function
    - ldb ; Load binary file
    - ldc ; Load code file

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#ifndef THRIVE_H
#define THRIVE_H

/* #############################################################################
 * # [SECTION] COMPILER SETTINGS
 * #############################################################################
 */
/* Check if using C99 or later (inline is supported) */
#if __STDC_VERSION__ >= 199901L
#define THRIVE_INLINE inline
#elif defined(__GNUC__) || defined(__clang__)
#define THRIVE_INLINE __inline__
#elif defined(_MSC_VER)
#define THRIVE_INLINE __inline
#else
#define THRIVE_INLINE
#endif

#define THRIVE_API static

typedef char s8;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef int i32;
typedef float f32;
typedef double f64;

#define THRIVE_STATIC_ASSERT(c, m) typedef char thrive_assert_##m[(c) ? 1 : -1]

THRIVE_STATIC_ASSERT(sizeof(s8) == 1, s8_size_must_be_1);
THRIVE_STATIC_ASSERT(sizeof(u8) == 1, u8_size_must_be_1);
THRIVE_STATIC_ASSERT(sizeof(u16) == 2, u16_size_must_be_2);
THRIVE_STATIC_ASSERT(sizeof(u32) == 4, u32_size_must_be_4);
THRIVE_STATIC_ASSERT(sizeof(i32) == 4, i32_size_must_be_4);
THRIVE_STATIC_ASSERT(sizeof(f32) == 4, f32_size_must_be_4);
THRIVE_STATIC_ASSERT(sizeof(f64) == 8, f64_size_must_be_8);

/* #############################################################################
 * # [SECTION] Helper Functions
 * #############################################################################
 */
THRIVE_API THRIVE_INLINE s8 thrive_char_is_digit(s8 c)
{
    return c >= '0' && c <= '9';
}

THRIVE_API THRIVE_INLINE s8 thrive_char_is_alpha(s8 c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

THRIVE_API THRIVE_INLINE u32 thrive_string_length(s8 *str)
{
    u32 len = 0;

    while (*str++)
    {
        ++len;
    }

    return len;
}

THRIVE_API THRIVE_INLINE u32 thrive_string_equals(s8 *a, s8 *b)
{
    while (*a && *b)
    {
        if (*a != *b)
        {
            return 0;
        }

        a++;
        b++;
    }

    return (*a == *b);
}

THRIVE_API THRIVE_INLINE u32 thrive_string_equals_length(s8 *a, s8 *b, u32 len)
{
    u32 i;

    for (i = 0; i < len; ++i)
    {
        if (a[i] != b[i])
        {
            return 0;
        }
    }

    return 1;
}

/* #############################################################################
 * # [SECTION] Thrive Status
 * #############################################################################
 */
typedef struct thrive_status
{
    enum type
    {
        THRIVE_STATUS_OK = 0,
        THRIVE_STATUS_ERROR_ARGUMENTS,
        THRIVE_STATUS_ERROR_SYNTAX

    } type;

    s8 *message;

} thrive_status;

/* #############################################################################
 * # [SECTION] Lexer
 * #############################################################################
 */
typedef enum thrive_token_kind
{
    THRIVE_TOKEN_KIND_EOF = 0,
    THRIVE_TOKEN_KIND_NEW_LINE,
    THRIVE_TOKEN_KIND_LPARAN,
    THRIVE_TOKEN_KIND_RPARAN,
    THRIVE_TOKEN_KIND_ASSIGN,
    THRIVE_TOKEN_KIND_ADD,
    THRIVE_TOKEN_KIND_SUB,
    THRIVE_TOKEN_KIND_MUL,
    THRIVE_TOKEN_KIND_DIV,
    THRIVE_TOKEN_KIND_INT,
    THRIVE_TOKEN_KIND_NAME,
    THRIVE_TOKEN_KIND_KEYWORD_RET,
    THRIVE_TOKEN_KIND_KEYWORD_U32,
    THRIVE_TOKEN_KIND_INVALID

} thrive_token_kind;

s8 *thrive_token_kind_names[] = {
    "EOF",
    "NEWLINE",
    "LPARAN",
    "RPARAN",
    "ASSIGN",
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "INT",
    "NAME",
    "KW_RET",
    "KW_U32",
    "INVALID"};

typedef struct thrive_token
{

    thrive_token_kind kind;

    s8 *start;
    s8 *end;

    union value
    {
        u32 number;
    } value;

} thrive_token;

typedef struct thrive_state
{
    s8 *source_code;
    u32 source_code_size;

    thrive_token current;

} thrive_state;

THRIVE_API THRIVE_INLINE thrive_token thrive_token_get(thrive_state *state)
{
    thrive_token token = {0};

    token.start = state->source_code;

    /* clang-format off */
    switch (*state->source_code)
    {   
        /* Whitespaces */
        case ' ': case '\r': case '\t':
        {
            while (*state->source_code == ' ' || *state->source_code == '\r' || *state->source_code == '\t') {
                state->source_code++;
            }

            return thrive_token_get(state);
        } 
        /* Line comments */
        case ';': 
        {
            while (*state->source_code && *state->source_code != '\n') {
                state->source_code++;
            }
            
            return thrive_token_get(state);
        }
        /* Number processing */
        case '0': case '1': case '2': case '3': case '4': case '5': case '6':
        case '7': case '8': case '9':
        {
            u32 value = 0;

            while (thrive_char_is_digit(*state->source_code)) {
                value *= 10;
                value += (u32) (*state->source_code++ - '0');
            }

            token.kind = THRIVE_TOKEN_KIND_INT;
            token.value.number = value;

            break;
        }
        /* Literal processing */
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
        case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case '_':
        {
            while (thrive_char_is_alpha(*state->source_code) ||
                   thrive_char_is_digit(*state->source_code) ||
                   *state->source_code == '_') 
            {
                state->source_code++;
            }

            token.kind = THRIVE_TOKEN_KIND_NAME;

            {
                u32 token_length = (u32) (state->source_code - token.start);

                if (thrive_string_equals_length("ret", token.start, token_length)) 
                {
                    token.kind = THRIVE_TOKEN_KIND_KEYWORD_RET;
                } 
                else if (thrive_string_equals_length("u32", token.start, token_length)) 
                {
                    token.kind = THRIVE_TOKEN_KIND_KEYWORD_U32;
                }
            }

            break;
        }
        /* Single char tokens */
        case '(':  { state->source_code++; token.kind = THRIVE_TOKEN_KIND_LPARAN;   break; }
        case ')':  { state->source_code++; token.kind = THRIVE_TOKEN_KIND_RPARAN;   break; }
        case '=':  { state->source_code++; token.kind = THRIVE_TOKEN_KIND_ASSIGN;   break; }
        case '+':  { state->source_code++; token.kind = THRIVE_TOKEN_KIND_ADD;      break; }
        case '-':  { state->source_code++; token.kind = THRIVE_TOKEN_KIND_SUB;      break; }
        case '*':  { state->source_code++; token.kind = THRIVE_TOKEN_KIND_MUL;      break; }
        case '/':  { state->source_code++; token.kind = THRIVE_TOKEN_KIND_DIV;      break; }
        case '\0': { state->source_code++; token.kind = THRIVE_TOKEN_KIND_EOF;      break; }
        case '\n': { state->source_code++; token.kind = THRIVE_TOKEN_KIND_NEW_LINE; break; }
        default:   { state->source_code++; token.kind = THRIVE_TOKEN_KIND_INVALID;  break; }
    }
    /* clang-format on */

    token.end = state->source_code;

    return token;
}

THRIVE_API void thrive_token_next(thrive_state *s)
{
    s->current = thrive_token_get(s);
}

THRIVE_API u8 thrive_token_accept(thrive_state *s, thrive_token_kind kind)
{
    if (s->current.kind == kind)
    {
        thrive_token_next(s);
        return 1;
    }
    return 0;
}

THRIVE_API u8 thrive_token_expect(thrive_state *s, thrive_token_kind kind)
{
    if (s->current.kind != kind)
    {
        return 0;
    }

    thrive_token_next(s);

    return 1;
}

THRIVE_API void thrive_token_skip_newlines(thrive_state *s)
{
    while (s->current.kind == THRIVE_TOKEN_KIND_NEW_LINE)
    {
        thrive_token_next(s);
    }
}

/* #############################################################################
 * # [SECTION] AST Parser
 * #############################################################################
 */
typedef enum thrive_ast_kind
{
    THRIVE_AST_INT,
    THRIVE_AST_NAME,
    THRIVE_AST_BINARY,
    THRIVE_AST_RETURN,
    THRIVE_AST_ASSIGN,
    THRIVE_AST_DECL,
    THRIVE_AST_BLOCK

} thrive_ast_kind;

typedef struct thrive_ast thrive_ast;

#define THRIVE_BLOCK_MAX 256

typedef struct thrive_ast_block
{
    thrive_ast *items[THRIVE_BLOCK_MAX];
    u32 count;
} thrive_ast_block;

struct thrive_ast
{
    thrive_ast_kind kind;

    union
    {
        u32 int_value;

        struct
        {
            s8 *start;
            u32 length;
        } name;

        struct
        {
            thrive_token_kind op;
            thrive_ast *left;
            thrive_ast *right;
        } binary;

        struct
        {
            thrive_ast *expr;
        } ret;

        struct
        {
            thrive_ast *left;
            thrive_ast *right;
        } assign;

        struct
        {
            thrive_ast *name;
            thrive_ast *value;
        } decl;

        thrive_ast_block block;

    } data;
};

#define THRIVE_AST_MAX 1024

static thrive_ast thrive_ast_pool[THRIVE_AST_MAX];
static u32 thrive_ast_count = 0;

THRIVE_API thrive_ast *thrive_ast_new(void)
{
    if (thrive_ast_count >= THRIVE_AST_MAX)
    {
        /* TODO: handle overflow properly */
        return 0;
    }

    return &thrive_ast_pool[thrive_ast_count++];
}

thrive_ast *thrive_ast_parse_expr(thrive_state *s);

thrive_ast *thrive_ast_parse_primary(thrive_state *s)
{
    thrive_token tok = s->current;

    if (tok.kind == THRIVE_TOKEN_KIND_INT)
    {
        thrive_ast *node = thrive_ast_new();
        node->kind = THRIVE_AST_INT;
        node->data.int_value = tok.value.number;

        thrive_token_next(s);
        return node;
    }

    if (tok.kind == THRIVE_TOKEN_KIND_NAME)
    {
        thrive_ast *node = thrive_ast_new();
        node->kind = THRIVE_AST_NAME;
        node->data.name.start = tok.start;
        node->data.name.length = (u32)(tok.end - tok.start);

        thrive_token_next(s);
        return node;
    }

    if (thrive_token_accept(s, THRIVE_TOKEN_KIND_LPARAN))
    {
        thrive_ast *expr = thrive_ast_parse_expr(s);
        thrive_token_expect(s, THRIVE_TOKEN_KIND_RPARAN);
        return expr;
    }

    return 0;
}

thrive_ast *thrive_ast_parse_mul(thrive_state *s)
{
    thrive_ast *left = thrive_ast_parse_primary(s);

    while (s->current.kind == THRIVE_TOKEN_KIND_MUL ||
           s->current.kind == THRIVE_TOKEN_KIND_DIV)
    {
        thrive_token_kind op = s->current.kind;
        thrive_ast *right;
        thrive_ast *node;

        thrive_token_next(s);

        right = thrive_ast_parse_primary(s);

        node = thrive_ast_new();
        node->kind = THRIVE_AST_BINARY;
        node->data.binary.op = op;
        node->data.binary.left = left;
        node->data.binary.right = right;

        left = node;
    }

    return left;
}

thrive_ast *thrive_ast_parse_add(thrive_state *s)
{
    thrive_ast *left = thrive_ast_parse_mul(s);

    while (s->current.kind == THRIVE_TOKEN_KIND_ADD ||
           s->current.kind == THRIVE_TOKEN_KIND_SUB)
    {
        thrive_token_kind op = s->current.kind;
        thrive_ast *right;
        thrive_ast *node;

        thrive_token_next(s);

        right = thrive_ast_parse_mul(s);

        node = thrive_ast_new();
        node->kind = THRIVE_AST_BINARY;
        node->data.binary.op = op;
        node->data.binary.left = left;
        node->data.binary.right = right;

        left = node;
    }

    return left;
}

thrive_ast *thrive_ast_parse_assign(thrive_state *s)
{
    thrive_ast *left = thrive_ast_parse_add(s);

    if (s->current.kind == THRIVE_TOKEN_KIND_ASSIGN)
    {
        thrive_ast *right;
        thrive_ast *node;

        thrive_token_next(s);

        right = thrive_ast_parse_assign(s); /* right-associative */

        node = thrive_ast_new();
        node->kind = THRIVE_AST_ASSIGN;
        node->data.assign.left = left;
        node->data.assign.right = right;

        return node;
    }

    return left;
}

thrive_ast *thrive_ast_parse_expr(thrive_state *s)
{
    return thrive_ast_parse_assign(s);
}

thrive_ast *thrive_ast_parse_statement(thrive_state *s)
{
    /* return */
    if (thrive_token_accept(s, THRIVE_TOKEN_KIND_KEYWORD_RET))
    {
        thrive_ast *node = thrive_ast_new();
        node->kind = THRIVE_AST_RETURN;
        node->data.ret.expr = thrive_ast_parse_expr(s);
        return node;
    }

    /* declaration: u32 a = expr */
    if (thrive_token_accept(s, THRIVE_TOKEN_KIND_KEYWORD_U32))
    {
        thrive_token name_tok = s->current;
        thrive_ast *name;
        thrive_ast *value;
        thrive_ast *node;

        thrive_token_expect(s, THRIVE_TOKEN_KIND_NAME);

        name = thrive_ast_new();
        name->kind = THRIVE_AST_NAME;
        name->data.name.start = name_tok.start;
        name->data.name.length = (u32)(name_tok.end - name_tok.start);

        value = 0;

        if (thrive_token_accept(s, THRIVE_TOKEN_KIND_ASSIGN))
        {
            value = thrive_ast_parse_expr(s);
        }

        node = thrive_ast_new();
        node->kind = THRIVE_AST_DECL;
        node->data.decl.name = name;
        node->data.decl.value = value;

        return node;
    }

    return thrive_ast_parse_expr(s);
}

thrive_ast *thrive_ast_parse_program(thrive_state *s)
{
    thrive_ast *node = thrive_ast_new();
    node->kind = THRIVE_AST_BLOCK;
    node->data.block.count = 0;

    thrive_token_next(s);
    thrive_token_skip_newlines(s);

    while (s->current.kind != THRIVE_TOKEN_KIND_EOF)
    {
        thrive_ast *stmt;

        if (node->data.block.count >= THRIVE_BLOCK_MAX)
        {
            /* TODO: overflow */
            break;
        }

        stmt = thrive_ast_parse_statement(s);

        node->data.block.items[node->data.block.count++] = stmt;

        thrive_token_skip_newlines(s);
    }

    return node;
}

thrive_ast *thrive_ast_parse(thrive_state *s)
{
    thrive_ast_count = 0;
    return thrive_ast_parse_program(s);
}

thrive_ast *thrive_ast_fold(thrive_ast *node)
{

    if (!node)
    {
        return 0;
    }

    switch (node->kind)
    {
    case THRIVE_AST_BINARY:
    {
        thrive_ast *l;
        thrive_ast *r;

        node->data.binary.left = thrive_ast_fold(node->data.binary.left);
        node->data.binary.right = thrive_ast_fold(node->data.binary.right);

        l = node->data.binary.left;
        r = node->data.binary.right;

        if (l && r && l->kind == THRIVE_AST_INT && r->kind == THRIVE_AST_INT)
        {
            u32 a = l->data.int_value;
            u32 b = r->data.int_value;
            u32 result = 0;

            switch (node->data.binary.op)
            {
            case THRIVE_TOKEN_KIND_ADD:
                result = a + b;
                break;
            case THRIVE_TOKEN_KIND_SUB:
                result = a - b;
                break;
            case THRIVE_TOKEN_KIND_MUL:
                result = a * b;
                break;
            case THRIVE_TOKEN_KIND_DIV:
                if (b != 0)
                    result = a / b;
                else
                    return node;
                break;
            default:
                return node;
            }

            node->kind = THRIVE_AST_INT;
            node->data.int_value = result;

            return node;
        }

        return node;
    }

    case THRIVE_AST_ASSIGN:
        node->data.assign.right = thrive_ast_fold(node->data.assign.right);
        return node;

    case THRIVE_AST_DECL:
        if (node->data.decl.value)
        {
            node->data.decl.value = thrive_ast_fold(node->data.decl.value);
        }
        return node;

    case THRIVE_AST_RETURN:
        node->data.ret.expr = thrive_ast_fold(node->data.ret.expr);
        return node;

    case THRIVE_AST_BLOCK:
    {
        u32 i;

        for (i = 0; i < node->data.block.count; ++i)
        {
            node->data.block.items[i] = thrive_ast_fold(node->data.block.items[i]);
        }
        return node;
    }

    default:
        return node;
    }
}

#endif /* THRIVE_H */

/*
 ------------------------------------------------------------------------------
 This software is available under 2 licenses -- choose whichever you prefer.
 ------------------------------------------------------------------------------
 ALTERNATIVE A - MIT License
 Copyright (c) 2026 nickscha
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
