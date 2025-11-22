/* thrive.h - v0.2 - public domain data structures - nickscha 2025

A C89 standard compliant, nostdlib (no C Standard Library) Low Level Programming Language inbetween Assembly and C (THRIVE).

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

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef int i32;
typedef float f32;
typedef double f64;

#define THRIVE_STATIC_ASSERT(c, m) typedef char thrive_assert_##m[(c) ? 1 : -1]

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
THRIVE_API THRIVE_INLINE u8 thrive_is_digit(u8 c)
{
    return c >= '0' && c <= '9';
}

THRIVE_API THRIVE_INLINE u32 thrive_strlen(u8 *str)
{
    u32 len = 0;

    while (*str++)
    {
        ++len;
    }

    return len;
}

THRIVE_API THRIVE_INLINE u32 thrive_streq(u8 *a, u8 *b)
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

THRIVE_API THRIVE_INLINE i32 thrive_strtol(u8 *str, u8 **endptr, i32 base)
{
    i32 result = 0;
    i32 sign = 1;

    if (*str == '-')
    {
        sign = -1;
        str++;
    }
    else if (*str == '+')
    {
        str++;
    }

    if (base == 0)
    {
        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
        {
            base = 16;
            str += 2;
        }
        else if (str[0] == '0' && (str[1] == 'b' || str[1] == 'B'))
        {
            base = 2;
            str += 2;
        }
        else if (str[0] == '0')
        {
            base = 8;
            str++;
        }
        else
        {
            base = 10;
        }
    }

    while (*str)
    {
        i32 digit;

        if (*str == '_')
        {
            str++;
            continue;
        }

        /* skip separator */
        if (*str >= '0' && *str <= '9')
        {
            digit = *str - '0';
        }
        else if (*str >= 'a' && *str <= 'f')
        {
            digit = 10 + (*str - 'a');
        }
        else if (*str >= 'A' && *str <= 'F')
        {
            digit = 10 + (*str - 'A');
        }
        else
        {
            break;
        }

        if (digit >= base)
        {
            break;
        }

        result = result * base + digit;
        str++;
    }

    if (endptr)
    {
        *endptr = (u8 *)str;
    }

    return sign * result;
}

THRIVE_API THRIVE_INLINE f64 thrive_strtod(u8 *str, u8 **endptr)
{
    f64 result = 0.0;
    f64 sign = 1.0;
    i32 exponent = 0, exp_sign = 1;
    f64 pow10 = 1.0;
    i32 i;

    if (*str == '-')
    {
        sign = -1.0;
        str++;
    }
    else if (*str == '+')
    {
        str++;
    }

    /* Integer part */
    while (thrive_is_digit(*str) || *str == '_')
    {
        if (*str == '_')
        {
            str++;
            continue;
        }

        result = result * 10.0 + (*str - '0');
        str++;
    }

    /* Fraction */
    if (*str == '.')
    {
        f64 base = 0.1;

        str++;

        while (thrive_is_digit(*str) || *str == '_')
        {
            if (*str == '_')
            {
                str++;
                continue;
            }

            result += (*str - '0') * base;
            base *= 0.1;

            str++;
        }
    }

    /* Exponent */
    if (*str == 'e' || *str == 'E')
    {
        str++;

        if (*str == '-')
        {
            exp_sign = -1;
            str++;
        }
        else if (*str == '+')
        {
            str++;
        }

        while (thrive_is_digit(*str) || *str == '_')
        {
            if (*str == '_')
            {
                str++;

                continue;
            }
            exponent = exponent * 10 + (*str - '0');
            str++;
        }
    }

    if (endptr)
    {
        *endptr = (u8 *)str;
    }

    /* Apply exponent */
    for (i = 0; i < exponent; ++i)
    {
        pow10 *= 10.0;
    }

    if (exp_sign < 0)
    {
        result /= pow10;
    }
    else
    {
        result *= pow10;
    }

    return sign * result;
}

/* #############################################################################
 * # [SECTION] Tokenizer
 * #############################################################################
 */
typedef enum thrive_token_type
{
    /* Identifier */
    THRIVE_TOKEN_VAR,

    /* Literals */
    THRIVE_TOKEN_INTEGER,
    THRIVE_TOKEN_FLOAT,
    THRIVE_TOKEN_STRING,
    THRIVE_TOKEN_LPAREN, /* ( */
    THRIVE_TOKEN_RPAREN, /* ) */

    /* Operator */
    THRIVE_TOKEN_ADD,    /* + */
    THRIVE_TOKEN_SUB,    /* - */
    THRIVE_TOKEN_MUL,    /* * */
    THRIVE_TOKEN_DIV,    /* / */
    THRIVE_TOKEN_ASSIGN, /* = */

    /* Keywords */
    THRIVE_TOKEN_KEYWORD_U32, /* unsigned 32bit integer */
    THRIVE_TOKEN_KEYWORD_RET, /* ret */
    THRIVE_TOKEN_KEYWORD_EXT, /* ext = external reference */

    THRIVE_TOKEN_EOF /* end of file */

} thrive_token_type;

static char *thrive_token_type_names[] = {
    "THRIVE_TOKEN_VAR",
    "THRIVE_TOKEN_INTEGER",
    "THRIVE_TOKEN_FLOAT",
    "THRIVE_TOKEN_STRING",
    "THRIVE_TOKEN_LPAREN",
    "THRIVE_TOKEN_RPAREN",
    "THRIVE_TOKEN_ADD",
    "THRIVE_TOKEN_SUB",
    "THRIVE_TOKEN_MUL",
    "THRIVE_TOKEN_DIV",
    "THRIVE_TOKEN_ASSIGN",
    "THRIVE_TOKEN_KEYWORD_U32",
    "THRIVE_TOKEN_KEYWORD_RET",
    "THRIVE_TOKEN_KEYWORD_EXT",
    "THRIVE_TOKEN_EOF"};

typedef struct thrive_token
{
    thrive_token_type type;

    u32 cursor_pos; /* Absolute position (0-based index) of the token start in the source code */
    u32 line_num;   /* Line number (1-based index) where the token starts */

    union value
    {
        i32 integer;  /* valid if THRIVE_TOKEN_INTEGER */
        f64 floating; /* valid if THRIVE_TOKEN_FLOAT   */

        /* valid if THRIVE_TOKEN_VAR or THRIVE_TOKEN_STRING */
        struct view
        {
            u8 *start;
            u32 length;

        } view;

    } value;

} thrive_token;

THRIVE_API THRIVE_INLINE u8 thrive_tokenizer(
    u8 *code,             /* The source code to tokenize */
    u32 code_size,        /* The source code size */
    thrive_token *tokens, /* The token buffer */
    u32 tokens_capacity,  /* The maximum capacity of tokens */
    u32 *tokens_size      /* Token size output */
)
{
    u8 *code_end;
    u8 *cursor;
    u32 line_num = 1;

    /* Validate inputs */
    if (!code || code_size <= 0 || !tokens || tokens_capacity <= 0 || !tokens_size)
    {
        return 0;
    }

    *tokens_size = 0;
    cursor = code;
    code_end = code + code_size;

    while (cursor < code_end)
    {
        u8 c = *cursor;

        /* 1. Fast Skip Whitespace */
        if (c <= ' ')
        {
            if (c == ' ' || c == '\t' || c == '\r')
            {
                cursor++;
                continue;
            }
            else if (c == '\n')
            {
                line_num++;
                cursor++;
                continue;
            }

            /* Skip other non-printable control characters */
            cursor++;

            continue;
        }

        if (cursor >= code_end)
        {
            break;
        }

        if (*tokens_size >= tokens_capacity)
        {
            return 0;
        }

        /* 2. Identifiers (alpha or _) */
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
        {
            thrive_token *tok = &tokens[*tokens_size];
            u8 *start = cursor;

            tok->cursor_pos = (u32)(cursor - code);
            tok->line_num = line_num;

            while (cursor < code_end)
            {
                u8 nc = *cursor;

                if (!((nc >= 'a' && nc <= 'z') ||
                      (nc >= 'A' && nc <= 'Z') ||
                      (nc >= '0' && nc <= '9') ||
                      nc == '_'))
                {
                    break;
                }

                cursor++;
            }

            tok->value.view.start = start;
            tok->value.view.length = (u32)(cursor - start);
            tok->type = THRIVE_TOKEN_VAR;

            if (tok->value.view.length == 3)
            {
                if (start[0] == 'u' && start[1] == '3' && start[2] == '2')
                {
                    tok->type = THRIVE_TOKEN_KEYWORD_U32;
                }
                else if (start[0] == 'r' && start[1] == 'e' && start[2] == 't')
                {
                    tok->type = THRIVE_TOKEN_KEYWORD_RET;
                }
                else if (start[0] == 'e' && start[1] == 'x' && start[2] == 't')
                {
                    tok->type = THRIVE_TOKEN_KEYWORD_EXT;
                }
            }

            (*tokens_size)++;

            continue;
        }

        /* 3. Numbers (Starts with Digit or Dot) */
        if ((c >= '0' && c <= '9') || c == '.')
        {
            thrive_token *tok = &tokens[*tokens_size];
            u8 *start = cursor;
            i32 is_float = (c == '.');

            tok->cursor_pos = (u32)(cursor - code);
            tok->line_num = line_num;

            /* Check Hex/Binary Prefix */
            if (c == '0' && cursor + 1 < code_end)
            {
                u8 next_c = *(cursor + 1);

                if (next_c == 'x' || next_c == 'X')
                {
                    /* Hex Parser */
                    i32 val = 0;
                    cursor += 2;

                    while (cursor < code_end)
                    {
                        u8 h = *cursor;

                        if (h == '_')
                        {
                            cursor++;
                            continue;
                        }

                        if (h >= '0' && h <= '9')
                        {
                            val = val * 16 + (h - '0');
                        }
                        else if (h >= 'a' && h <= 'f')
                        {
                            val = val * 16 + (h - 'a' + 10);
                        }
                        else if (h >= 'A' && h <= 'F')
                        {
                            val = val * 16 + (h - 'A' + 10);
                        }
                        else
                        {
                            break;
                        }

                        cursor++;
                    }

                    tok->type = THRIVE_TOKEN_INTEGER;
                    tok->value.integer = val;

                    (*tokens_size)++;

                    continue;
                }

                if (next_c == 'b' || next_c == 'B')
                {
                    /* Binary Parser */
                    i32 val = 0;
                    cursor += 2;

                    while (cursor < code_end)
                    {
                        u8 h = *cursor;

                        if (h == '_')
                        {
                            cursor++;
                            continue;
                        }

                        if (h == '0' || h == '1')
                        {
                            val = (val << 1) | (h - '0');
                        }
                        else
                        {
                            break;
                        }

                        cursor++;
                    }
                    tok->type = THRIVE_TOKEN_INTEGER;
                    tok->value.integer = val;

                    (*tokens_size)++;

                    continue;
                }
            }

            /* Decimal / Float Parser  */
            while (cursor < code_end)
            {
                u8 n = *cursor;

                if ((n >= '0' && n <= '9') || n == '_')
                {
                    cursor++;
                    continue;
                }
                if (n == '.')
                {
                    is_float = 1;
                    cursor++;
                    continue;
                }
                if (n == 'e' || n == 'E')
                {
                    is_float = 1;
                    cursor++;

                    if (cursor < code_end && (*cursor == '+' || *cursor == '-'))
                    {
                        cursor++;
                    }
                    continue;
                }
                /* Float suffix */
                if (n == 'f' || n == 'F')
                {
                    is_float = 1;
                    cursor++;
                }
                break;
            }

            if (is_float)
            {
                tok->type = THRIVE_TOKEN_FLOAT;
                tok->value.floating = thrive_strtod(start, (void *)0);
            }
            else
            {
                tok->type = THRIVE_TOKEN_INTEGER;
                tok->value.integer = thrive_strtol(start, (void *)0, 10);
            }

            (*tokens_size)++;

            continue;
        }

        /* 4. String */
        if (c == '"')
        {
            thrive_token *tok = &tokens[*tokens_size];
            u8 *start;

            tok->cursor_pos = (u32)(cursor - code);
            tok->line_num = line_num;

            cursor++; /* skip opening quote */
            start = cursor;

            while (cursor < code_end && *cursor != '"')
            {
                if (*cursor == '\\' && cursor + 1 < code_end)
                {
                    cursor++;
                }
                cursor++;
            }

            tok->type = THRIVE_TOKEN_STRING;
            tok->value.view.start = start;
            tok->value.view.length = (u32)(cursor - start);

            if (cursor < code_end && *cursor == '"')
            {
                cursor++; /* skip closing */
            }

            (*tokens_size)++;

            continue;
        }

        /* 5. Operators & Punctuation */
        {
            thrive_token *tok = &tokens[*tokens_size];

            tok->cursor_pos = (u32)(cursor - code);
            tok->line_num = line_num;

            switch (c)
            {
            case '+':
                tok->type = THRIVE_TOKEN_ADD;
                (*tokens_size)++;
                cursor++;
                continue;
            case '-':
                tok->type = THRIVE_TOKEN_SUB;
                (*tokens_size)++;
                cursor++;
                continue;
            case '*':
                tok->type = THRIVE_TOKEN_MUL;
                (*tokens_size)++;
                cursor++;
                continue;
            case '/':
                tok->type = THRIVE_TOKEN_DIV;
                (*tokens_size)++;
                cursor++;
                continue;
            case '=':
                tok->type = THRIVE_TOKEN_ASSIGN;
                (*tokens_size)++;
                cursor++;
                continue;
            case '(':
                tok->type = THRIVE_TOKEN_LPAREN;
                (*tokens_size)++;
                cursor++;
                continue;
            case ')':
                tok->type = THRIVE_TOKEN_RPAREN;
                (*tokens_size)++;
                cursor++;
                continue;
            default:
                cursor++;
                break; /* Skip unknown chars / error handling */
            }
        }
    }

    /* End of File */
    {
        thrive_token *eof_tok = &tokens[*tokens_size];
        eof_tok->cursor_pos = (u32)(cursor - code);
        eof_tok->line_num = line_num;
        eof_tok->type = THRIVE_TOKEN_EOF;
        (*tokens_size)++;
    }

    return 1;
}

/* #############################################################################
 * # [SECTION] AST (Pratt Parsing)
 * #############################################################################
 */
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

        /* Check for closing parenthesis */
        if (!thrive_ast_accept(p, THRIVE_TOKEN_RPAREN))
        {
            /* TODO: Error handling */
            thrive_token *token = thrive_ast_peek(p);
            u8 *message = (u8 *)"Expected ')'";

            (void)token;
            (void)message;
        }

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

/* #############################################################################
 * # [SECTION] AST Optimizer
 * #############################################################################
 */
#define MAX_CONSTANTS 128
#define MAX_AST_NODES 1024

THRIVE_API u32 thrive_hash_name(u8 *name)
{
    u32 hash = 5381;
    u8 c;
    while ((c = *name++))
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % MAX_CONSTANTS;
}

typedef struct thrive_const_sym
{
    u8 name[32];
    u8 is_float;
    union
    {
        i32 i_val;
        f64 f_val;
    } val;
} thrive_const_sym;

typedef struct thrive_optimizer_ctx
{
    thrive_ast *ast;
    u32 ast_size;
    thrive_const_sym constants[MAX_CONSTANTS];
    u32 count;
} thrive_optimizer_ctx;

THRIVE_API thrive_const_sym *thrive_ast_find_const(thrive_optimizer_ctx *ctx, u8 *name)
{
    u32 initial_idx = thrive_hash_name(name);
    u32 idx = initial_idx;

    if (name[0] == 0)
    {
        return (thrive_const_sym *)((void *)0);
    }

    do
    {
        if (ctx->constants[idx].name[0] != 0 && thrive_streq(ctx->constants[idx].name, name))
        {
            return &ctx->constants[idx];
        }
        if (ctx->constants[idx].name[0] == 0)
        {
            return (thrive_const_sym *)((void *)0);
        }

        idx = (idx + 1) % MAX_CONSTANTS;

    } while (idx != initial_idx);

    return (thrive_const_sym *)((void *)0);
}

THRIVE_API void thrive_ast_register_const(thrive_optimizer_ctx *ctx, u8 *name, u8 is_float, i32 i_val, f64 f_val)
{
    u32 initial_idx = thrive_hash_name(name);
    u32 idx = initial_idx;
    u32 k = 0;

    if (ctx->count >= MAX_CONSTANTS)
    {
        return;
    }

    if (thrive_ast_find_const(ctx, name))
    {
        return;
    }

    do
    {
        if (ctx->constants[idx].name[0] == 0)
        {
            while (name[k] && k < 31)
            {
                ctx->constants[idx].name[k] = name[k];
                k++;
            }
            ctx->constants[idx].name[k] = 0;
            ctx->constants[idx].is_float = is_float;

            if (is_float)
            {
                ctx->constants[idx].val.f_val = f_val;
            }
            else
            {
                ctx->constants[idx].val.i_val = i_val;
            }

            ctx->count++;

            return;
        }
        idx = (idx + 1) % MAX_CONSTANTS;

    } while (idx != initial_idx);
}

/* --- Constant Folding & Propagation Logic --- */
THRIVE_API u8 thrive_ast_try_propagate_var(thrive_optimizer_ctx *ctx, u16 node_id)
{
    thrive_ast *node = &ctx->ast[node_id];
    thrive_const_sym *sym;

    if (node->type != AST_VAR)
    {
        return 0;
    }

    sym = thrive_ast_find_const(ctx, node->v.name);
    if (sym)
    {
        if (sym->is_float)
        {
            node->type = AST_FLOAT;
            node->v.float_val = sym->val.f_val;
        }
        else
        {
            node->type = AST_INT;
            node->v.int_val = sym->val.i_val;
        }
        return 1;
    }
    return 0;
}

THRIVE_API u8 thrive_ast_try_fold_binary(thrive_ast *ast, u16 node_id)
{
    thrive_ast *node = &ast[node_id];
    thrive_ast *left_node, *right_node;
    u8 left_is_lit, right_is_lit, result_is_float;
    i32 int_left_val, int_right_val;
    f64 float_left_val, float_right_val;

    if (node->type != AST_ADD && node->type != AST_SUB &&
        node->type != AST_MUL && node->type != AST_DIV)
    {
        return 0;
    }

    left_node = &ast[node->v.op.left];
    right_node = &ast[node->v.op.right];

    left_is_lit = (left_node->type == AST_INT || left_node->type == AST_FLOAT);
    right_is_lit = (right_node->type == AST_INT || right_node->type == AST_FLOAT);

    if (left_is_lit && right_is_lit)
    {
        result_is_float = (left_node->type == AST_FLOAT || right_node->type == AST_FLOAT);

        int_left_val = (left_node->type == AST_INT) ? left_node->v.int_val : 0;
        int_right_val = (right_node->type == AST_INT) ? right_node->v.int_val : 0;

        float_left_val = (left_node->type == AST_FLOAT) ? left_node->v.float_val : (f64)int_left_val;
        float_right_val = (right_node->type == AST_FLOAT) ? right_node->v.float_val : (f64)int_right_val;

        if (result_is_float)
        {
            f64 res = 0;

            switch (node->type)
            {
            case AST_ADD:
                res = float_left_val + float_right_val;
                break;
            case AST_SUB:
                res = float_left_val - float_right_val;
                break;
            case AST_MUL:
                res = float_left_val * float_right_val;
                break;
            case AST_DIV:
                if (float_right_val != 0)
                    res = float_left_val / float_right_val;
                break;
            default:
                break;
            }

            node->type = AST_FLOAT;
            node->v.float_val = res;
        }
        else
        {
            i32 res = 0;

            switch (node->type)
            {
            case AST_ADD:
                res = int_left_val + int_right_val;
                break;
            case AST_SUB:
                res = int_left_val - int_right_val;
                break;
            case AST_MUL:
                res = int_left_val * int_right_val;
                break;
            case AST_DIV:
                if (int_right_val != 0)
                    res = int_left_val / int_right_val;
                break;
            default:
                break;
            }

            node->type = AST_INT;
            node->v.int_val = res;
        }
        return 1;
    }
    return 0;
}

THRIVE_API void thrive_ast_optimize_node(thrive_optimizer_ctx *ctx, u16 node_id)
{
    thrive_ast *node;

    if (node_id >= ctx->ast_size)
    {
        return;
    }

    node = &ctx->ast[node_id];

    switch (node->type)
    {
    case AST_ADD:
    case AST_SUB:
    case AST_MUL:
    case AST_DIV:
    case AST_ASSIGN:
        thrive_ast_optimize_node(ctx, node->v.op.left);
        thrive_ast_optimize_node(ctx, node->v.op.right);
        thrive_ast_try_fold_binary(ctx->ast, node_id);
        break;
    case AST_DECL:
        thrive_ast_optimize_node(ctx, node->v.decl.expr);
        break;
    case AST_RETURN:
        thrive_ast_optimize_node(ctx, node->v.expr);
        break;
    case AST_VAR:
        thrive_ast_try_propagate_var(ctx, node_id);
        break;
    default:
        break;
    }
}

THRIVE_API void thrive_ast_scan_constants(thrive_optimizer_ctx *ctx)
{
    u32 i;
    thrive_ast *ast = ctx->ast;

    for (i = 0; i < ctx->ast_size; ++i)
    {
        if (ast[i].type == AST_DECL)
        {
            u16 expr_idx = ast[i].v.decl.expr;
            thrive_ast *expr_node = &ast[expr_idx];

            if (expr_node->type == AST_INT)
            {
                thrive_ast_register_const(ctx, ast[i].v.decl.name, 0, expr_node->v.int_val, 0.0);
            }
            else if (expr_node->type == AST_FLOAT)
            {
                thrive_ast_register_const(ctx, ast[i].v.decl.name, 1, 0, expr_node->v.float_val);
            }
        }
    }
}

/* --- General Dead Code Elimination (Mark-Sweep-Compact) --- */

THRIVE_API void thrive_ast_mark_alive(thrive_ast *ast, u8 *alive_map, u16 node_id)
{
    thrive_ast *node;

    /* If already marked, stop recursion */
    if (alive_map[node_id])
    {
        return;
    }

    alive_map[node_id] = 1;
    node = &ast[node_id];

    switch (node->type)
    {
    case AST_ADD:
    case AST_SUB:
    case AST_MUL:
    case AST_DIV:
    case AST_ASSIGN:
        thrive_ast_mark_alive(ast, alive_map, node->v.op.left);
        thrive_ast_mark_alive(ast, alive_map, node->v.op.right);
        break;
    case AST_DECL:
        thrive_ast_mark_alive(ast, alive_map, node->v.decl.expr);
        break;
    case AST_RETURN:
        thrive_ast_mark_alive(ast, alive_map, node->v.expr);
        break;
    default:
        break;
    }
}

THRIVE_API void thrive_ast_compact(thrive_ast *ast, u32 *ast_size)
{
    /* C89 requires variables at start */
    u8 alive_map[MAX_AST_NODES];  /* 1 if node is used, 0 otherwise */
    u16 index_map[MAX_AST_NODES]; /* Maps old_index -> new_index */
    u32 i, write_idx;
    u32 old_size = *ast_size;
    thrive_ast *node;

    /* 1. Initialize maps */
    for (i = 0; i < old_size; ++i)
    {
        alive_map[i] = 0;
        index_map[i] = 0;
    }

    /* 2. Mark Phase: Identify all live nodes.
     * Roots are: AST_RETURN, AST_ASSIGN, and any "bare" expressions if your language supports them.
     * AST_DECL nodes are considered "roots" only if their variables were NOT propagated (simplified here: we assume constant prop handles declarations,
     * but to be safe for "multi argument" or non-declarations, we should mark them if they are not pure constants).
     * For strict DCE of constants: We only mark non-DECL statements as roots.
     */
    for (i = 0; i < old_size; ++i)
    {
        if (ast[i].type == AST_RETURN || ast[i].type == AST_ASSIGN)
        {
            thrive_ast_mark_alive(ast, alive_map, (u16)i);
        }
    }

    /* 3. Compaction Phase: Move live nodes to front */
    write_idx = 0;

    for (i = 0; i < old_size; ++i)
    {
        if (alive_map[i])
        {
            if (i != write_idx)
            {
                ast[write_idx] = ast[i];
            }

            index_map[i] = (u16)write_idx;
            write_idx++;
        }
    }

    /* 4. Relink Phase: Update indices in moved nodes */
    for (i = 0; i < write_idx; ++i)
    {
        node = &ast[i];

        switch (node->type)
        {
        case AST_ADD:
        case AST_SUB:
        case AST_MUL:
        case AST_DIV:
        case AST_ASSIGN:
            node->v.op.left = index_map[node->v.op.left];
            node->v.op.right = index_map[node->v.op.right];
            break;
        case AST_DECL:
            node->v.decl.expr = index_map[node->v.decl.expr];
            break;
        case AST_RETURN:
            node->v.expr = index_map[node->v.expr];
            break;
        default:
            break;
        }
    }

    *ast_size = write_idx;
}

THRIVE_API void thrive_ast_optimize(thrive_ast *ast, u32 *ast_size_ptr)
{
    thrive_optimizer_ctx ctx;
    u32 i;

    ctx.ast = ast;
    ctx.ast_size = *ast_size_ptr;
    ctx.count = 0;

    for (i = 0; i < MAX_CONSTANTS; ++i)
    {
        ctx.constants[i].name[0] = 0;
    }

    /* Pass 1 & 2: Scan and Propagate (First Pass) */
    thrive_ast_scan_constants(&ctx);

    for (i = 0; i < ctx.ast_size; ++i)
    {
        if (ast[i].type == AST_DECL || ast[i].type == AST_ASSIGN || ast[i].type == AST_RETURN)
        {
            thrive_ast_optimize_node(&ctx, (u16)i);
        }
    }

    /* Pass 3 & 4: Rescan and Propagate (Second Pass for deep folding) */
    thrive_ast_scan_constants(&ctx);
    for (i = 0; i < ctx.ast_size; ++i)
    {
        /* Primarily target returns for final propagation */
        if (ast[i].type == AST_RETURN)
        {
            thrive_ast_optimize_node(&ctx, (u16)i);
        }
    }

    /* Pass 5: General Dead Code Elimination */
    thrive_ast_compact(ast, ast_size_ptr);
}

/* #############################################################################
 * # [SECTION] Codegen x86_64 (NASM) Win32
 * #############################################################################
 */
#define MAX_GLOBALS 128

typedef enum thrive_sym_section
{
    SECTION_BSS,
    SECTION_DATA
} thrive_sym_section;

typedef struct thrive_symbol
{
    u8 name[32];
    thrive_sym_section section;
    i32 initial_value; /* Only valid if SECTION_DATA */
} thrive_symbol;

typedef struct thrive_codegen_ctx
{
    /* Output Buffer */
    u8 *buf;
    u32 cap;
    u32 size;

    /* AST & State */
    thrive_ast *ast;
    u32 ast_size;
    thrive_symbol globals[MAX_GLOBALS];
    u32 globals_count;

} thrive_codegen_ctx;

THRIVE_API void thrive_cg_emit_char(thrive_codegen_ctx *ctx, u8 c)
{
    if (ctx->size < ctx->cap)
    {
        ctx->buf[ctx->size] = c;
        ctx->size++;
    }
    if (ctx->size < ctx->cap)
    {
        ctx->buf[ctx->size] = 0;
    }
    else if (ctx->cap > 0)
    {
        ctx->buf[ctx->cap - 1] = 0;
    }
}

THRIVE_API void thrive_cg_emit_str(thrive_codegen_ctx *ctx, char *s)
{
    while (*s)
    {
        thrive_cg_emit_char(ctx, (u8)*s);
        s++;
    }
}

THRIVE_API void thrive_cg_emit_u32(thrive_codegen_ctx *ctx, u32 v)
{
    u8 buffer[16];
    i32 i = 0;
    i32 j;

    if (v == 0)
    {
        thrive_cg_emit_char(ctx, '0');
        return;
    }

    while (v > 0)
    {
        buffer[i++] = (u8)((v % 10) + '0');
        v /= 10;
    }

    for (j = i - 1; j >= 0; --j)
    {
        thrive_cg_emit_char(ctx, buffer[j]);
    }
}

THRIVE_API void thrive_cg_emit_i32(thrive_codegen_ctx *ctx, i32 v)
{
    if (v < 0)
    {
        thrive_cg_emit_char(ctx, '-');
        v = -v;
    }

    thrive_cg_emit_u32(ctx, (u32)v);
}

THRIVE_API void thrive_cg_emit_hex_digit(thrive_codegen_ctx *ctx, u8 nibble)
{
    if (nibble < 10)
    {
        thrive_cg_emit_char(ctx, nibble + '0');
    }
    else
    {
        thrive_cg_emit_char(ctx, nibble - 10 + 'A');
    }
}

THRIVE_API void thrive_cg_emit_hex_u32_full(thrive_codegen_ctx *ctx, u32 v)
{
    i32 i;

    for (i = 28; i >= 0; i -= 4)
    {
        thrive_cg_emit_hex_digit(ctx, (u8)((v >> i) & 0xF));
    }
}

THRIVE_API thrive_symbol *thrive_find_global(thrive_codegen_ctx *ctx, u8 *name)
{
    u32 i;

    for (i = 0; i < ctx->globals_count; ++i)
    {
        if (thrive_streq(ctx->globals[i].name, name))
        {
            return &ctx->globals[i];
        }
    }

    return (thrive_symbol *)((void *)0);
}

THRIVE_API void thrive_register_global(thrive_codegen_ctx *ctx, u8 *name, thrive_sym_section sec, i32 val)
{
    u32 idx;
    u32 k = 0;

    if (thrive_find_global(ctx, name))
    {
        return; /* Already exists */
    }

    if (ctx->globals_count >= MAX_GLOBALS)
    {
        return;
    }

    idx = ctx->globals_count++;

    while (name[k] && k < 31)
    {
        ctx->globals[idx].name[k] = name[k];
        k++;
    }

    ctx->globals[idx].name[k] = 0;
    ctx->globals[idx].section = sec;
    ctx->globals[idx].initial_value = val;
}

/* ---- Pass 1: Scan for Variables ---- */
THRIVE_API void thrive_emit_node(thrive_codegen_ctx *ctx, u16 node_idx)
{
    thrive_ast *node = &ctx->ast[node_idx];

    switch (node->type)
    {
    case AST_INT:
        thrive_cg_emit_str(ctx, "    mov  rax, ");
        thrive_cg_emit_i32(ctx, node->v.int_val);
        thrive_cg_emit_str(ctx, "\n    push rax\n");
        break;

    case AST_FLOAT:
    {
        union
        {
            f64 f;
            u32 u[2];

        } cvt;

        cvt.f = node->v.float_val;

        thrive_cg_emit_str(ctx, "    mov  rax, 0x");
        thrive_cg_emit_hex_u32_full(ctx, cvt.u[1]);
        thrive_cg_emit_hex_u32_full(ctx, cvt.u[0]);
        thrive_cg_emit_str(ctx, " ; float hex\n    push rax\n");
        break;
    }

    case AST_VAR:
    {
        /* Load from Global/Static address */
        thrive_cg_emit_str(ctx, "    mov  rax, [rel ");
        thrive_cg_emit_str(ctx, (char *)node->v.name);
        thrive_cg_emit_str(ctx, "]\n    push rax\n");
        break;
    }

    case AST_ADD:
        thrive_emit_node(ctx, node->v.op.left);
        thrive_emit_node(ctx, node->v.op.right);
        thrive_cg_emit_str(ctx, "    pop  rbx\n    pop  rax\n    add  rax, rbx\n    push rax\n");
        break;

    case AST_SUB:
        thrive_emit_node(ctx, node->v.op.left);
        thrive_emit_node(ctx, node->v.op.right);
        thrive_cg_emit_str(ctx, "    pop  rbx\n    pop  rax\n    sub  rax, rbx\n    push rax\n");
        break;

    case AST_MUL:
        thrive_emit_node(ctx, node->v.op.left);
        thrive_emit_node(ctx, node->v.op.right);
        thrive_cg_emit_str(ctx, "    pop  rbx\n    pop  rax\n    imul rax, rbx\n    push rax\n");
        break;

    case AST_DIV:
        thrive_emit_node(ctx, node->v.op.left);
        thrive_emit_node(ctx, node->v.op.right);
        thrive_cg_emit_str(ctx, "    pop  rbx\n    pop  rax\n    cqo\n    idiv rbx\n    push rax\n");
        break;

    case AST_DECL:
    {
        /* Check if this variable was optimized to .data section */
        thrive_symbol *sym = thrive_find_global(ctx, node->v.decl.name);
        if (sym && sym->section == SECTION_DATA)
        {
            /* It is already initialized in .data, do nothing in .text!
            thrive_cg_emit_str(ctx, "    ; Decl ");
            thrive_cg_emit_str(ctx, (char*)sym->name);
            thrive_cg_emit_str(ctx, " is in .data\n");
            */
            break;
        }

        /* Otherwise, generate runtime code and store in .bss location */
        thrive_emit_node(ctx, node->v.decl.expr);
        thrive_cg_emit_str(ctx, "    pop  rax\n    mov  [rel ");
        thrive_cg_emit_str(ctx, (char *)node->v.decl.name);
        thrive_cg_emit_str(ctx, "], rax\n");
        break;
    }

    case AST_ASSIGN:
    {
        thrive_ast *left_node = &ctx->ast[node->v.op.left];
        if (left_node->type == AST_VAR)
        {
            thrive_emit_node(ctx, node->v.op.right);
            thrive_cg_emit_str(ctx, "    pop  rax\n    mov  [rel ");
            thrive_cg_emit_str(ctx, (char *)left_node->v.name);
            thrive_cg_emit_str(ctx, "], rax\n");
        }
        break;
    }

    case AST_RETURN:
        thrive_emit_node(ctx, node->v.expr);
        thrive_cg_emit_str(ctx, "    pop  rcx\n    call ExitProcess\n");
        break;

    default:
        break;
    }
}

/* ---- Main Entry Point ---- */

THRIVE_API void thrive_codegen(
    thrive_ast *ast,
    u32 ast_size,
    u8 *code,
    u32 code_capacity,
    u32 *code_size)
{
    thrive_codegen_ctx ctx;
    u32 i;
    thrive_ast_type last_type = (thrive_ast_type)-1;

    ctx.buf = code;
    ctx.cap = code_capacity;
    ctx.size = 0;
    ctx.ast = ast;
    ctx.ast_size = ast_size;
    ctx.globals_count = 0;

    *code_size = 0;

    /* Ensure clean start */
    if (ctx.cap > 0)
    {
        ctx.buf[0] = 0;
    }

    thrive_cg_emit_str(&ctx, "bits 64\ndefault rel\n\n");

    /* --- Pass 1: Scan Symbols --- */
    for (i = 0; i < ast_size; ++i)
    {
        if (ast[i].type == AST_DECL)
        {
            /* Optimization: If init expr is just an INT, put in .data */
            u16 expr_idx = ast[i].v.decl.expr;

            if (ast[expr_idx].type == AST_INT)
            {
                thrive_register_global(&ctx, ast[i].v.decl.name, SECTION_DATA, ast[expr_idx].v.int_val);
            }
            else
            {
                thrive_register_global(&ctx, ast[i].v.decl.name, SECTION_BSS, 0);
            }
        }
    }

    /* --- Pass 2: Emit .data Section --- */
    thrive_cg_emit_str(&ctx, "segment .data\n");

    for (i = 0; i < ctx.globals_count; ++i)
    {
        if (ctx.globals[i].section == SECTION_DATA)
        {
            thrive_cg_emit_str(&ctx, "    ");
            thrive_cg_emit_str(&ctx, (char *)ctx.globals[i].name);
            thrive_cg_emit_str(&ctx, ": dq "); /* Using dq (64bit) for simplicity */
            thrive_cg_emit_i32(&ctx, ctx.globals[i].initial_value);
            thrive_cg_emit_char(&ctx, '\n');
        }
    }
    thrive_cg_emit_char(&ctx, '\n');

    /* --- Pass 3: Emit .bss Section --- */
    thrive_cg_emit_str(&ctx, "segment .bss\n");
    for (i = 0; i < ctx.globals_count; ++i)
    {
        if (ctx.globals[i].section == SECTION_BSS)
        {
            thrive_cg_emit_str(&ctx, "    ");
            thrive_cg_emit_str(&ctx, (char *)ctx.globals[i].name);
            thrive_cg_emit_str(&ctx, ": resq 1\n");
        }
    }
    thrive_cg_emit_char(&ctx, '\n');

    /* --- Pass 4: Emit .text Section --- */
    thrive_cg_emit_str(&ctx, "segment .text\nglobal main\nextern ExitProcess\n\nmain:\n");
    thrive_cg_emit_str(&ctx, "    sub rsp, 40 ; Shadow space (32) + Align (8)\n\n");

    for (i = 0; i < ast_size; ++i)
    {
        if (ast[i].type == AST_DECL ||
            ast[i].type == AST_ASSIGN ||
            ast[i].type == AST_RETURN)
        {
            thrive_emit_node(&ctx, (u16)i);
            last_type = ast[i].type;
        }
    }

    if (last_type != AST_RETURN)
    {
        thrive_cg_emit_str(&ctx, "    xor rcx, rcx\n    call ExitProcess\n");
    }

    *code_size = ctx.size;
}

#endif /* THRIVE_H */

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
