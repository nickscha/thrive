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
    - if/else
    - for
    - break
    - continue

SYNTAX (WIP)

  u32 a = 10
  u32 b = 0
  u32 i

  if (a > 5)
    a = 5
    b = 1
  else
    a = 3

  ; for loop example
  for (i = 0 : i < 10 : ++i)
    a += 1
    b += 1

    if (a > 7)
      break

  ; function declaration
  u32 sum(u32 a : u32 b)
    u32 result
    result = a + b
    ret result

  ; function call
  u32 res = sum(a : b)

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

/* #############################################################################
 * # [SECTION] 64 bit types
 * #############################################################################
 */
#if __STDC_VERSION__ >= 199901L
typedef long long i64;
typedef unsigned long long u64;
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlong-long"
typedef long long i64;
typedef unsigned long long u64;
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
typedef __int64 i64;
typedef unsigned __int64 u64;
#else
typedef long i64;
typedef unsigned long u64;
#endif

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

#undef THRIVE_STATIC_ASSERT

/* #############################################################################
 * # [SECTION] Structs anb Enums
 * #############################################################################
 */
typedef enum thrive_status_type
{
    THRIVE_STATUS_OK = 0,
    THRIVE_STATUS_ERROR_ARGUMENTS,
    THRIVE_STATUS_ERROR_SYNTAX,
    THRIVE_STATUS_ERROR_MEMORY

} thrive_status_type;

typedef struct thrive_status
{
    thrive_status_type type;

    s8 *message;

    s8 *token_start;
    s8 *token_end;

    u32 line;
    u32 column;

    s8 *line_start;

} thrive_status;

typedef enum thrive_token_kind
{
    THRIVE_TOKEN_KIND_EOF = 0,
    THRIVE_TOKEN_KIND_NEW_LINE,
    THRIVE_TOKEN_KIND_LPAREN,      /* ( */
    THRIVE_TOKEN_KIND_RPAREN,      /* ) */
    THRIVE_TOKEN_KIND_LBRACE,      /* { */
    THRIVE_TOKEN_KIND_RBRACE,      /* } */
    THRIVE_TOKEN_KIND_LBRACKET,    /* [ */
    THRIVE_TOKEN_KIND_RBRACKET,    /* ] */
    THRIVE_TOKEN_KIND_ASSIGN,      /* = */
    THRIVE_TOKEN_KIND_NEGATE,      /* ! */
    THRIVE_TOKEN_KIND_ADD,         /* + */
    THRIVE_TOKEN_KIND_SUB,         /* - */
    THRIVE_TOKEN_KIND_MUL,         /* * */
    THRIVE_TOKEN_KIND_DIV,         /* / */
    THRIVE_TOKEN_KIND_ADD_ASSIGN,  /* += */
    THRIVE_TOKEN_KIND_SUB_ASSIGN,  /* -= */
    THRIVE_TOKEN_KIND_MUL_ASSIGN,  /* *= */
    THRIVE_TOKEN_KIND_DIV_ASSIGN,  /* /= */
    THRIVE_TOKEN_KIND_INC,         /* ++ */
    THRIVE_TOKEN_KIND_DEC,         /* -- */
    THRIVE_TOKEN_KIND_EQUALS,      /* == */
    THRIVE_TOKEN_KIND_NOT_EQUALS,  /* != */
    THRIVE_TOKEN_KIND_LT,          /* < */
    THRIVE_TOKEN_KIND_GT,          /* > */
    THRIVE_TOKEN_KIND_LT_EQUALS,   /* <= */
    THRIVE_TOKEN_KIND_GT_EQUALS,   /* >= */
    THRIVE_TOKEN_KIND_OR_BITWISE,  /* | */
    THRIVE_TOKEN_KIND_OR_LOGICAL,  /* || */
    THRIVE_TOKEN_KIND_AND_BITWISE, /* & */
    THRIVE_TOKEN_KIND_AND_LOGICAL, /* && */
    THRIVE_TOKEN_KIND_XOR_BITWISE, /* ^ */
    THRIVE_TOKEN_KIND_NOT_BITWISE, /* ~ */
    THRIVE_TOKEN_KIND_LSHIFT,      /* << */
    THRIVE_TOKEN_KIND_RSHIFT,      /* >> */

    /* Tenary */
    THRIVE_TOKEN_KIND_QUESTION, /* ? */
    THRIVE_TOKEN_KIND_COLON,    /* : */

    THRIVE_TOKEN_KIND_INT,
    THRIVE_TOKEN_KIND_NAME,
    THRIVE_TOKEN_KIND_STRING,
    THRIVE_TOKEN_KIND_CHAR,
    THRIVE_TOKEN_KIND_TYPE_U32,
    THRIVE_TOKEN_KIND_TYPE_S8,
    THRIVE_TOKEN_KIND_KEYWORD_EXT,
    THRIVE_TOKEN_KIND_KEYWORD_RET,
    THRIVE_TOKEN_KIND_KEYWORD_IF,
    THRIVE_TOKEN_KIND_KEYWORD_ELSE,
    THRIVE_TOKEN_KIND_KEYWORD_FOR,
    THRIVE_TOKEN_KIND_KEYWORD_BREAK,
    THRIVE_TOKEN_KIND_KEYWORD_CONTINUE,
    THRIVE_TOKEN_KIND_INVALID

} thrive_token_kind;

s8 *thrive_token_kind_names[] = {
    "EOF",
    "NEW_LINE",
    "LPAREN",
    "RPAREN",
    "LBRACE",
    "RBRACE",
    "LBRACKET",
    "RBRACKET",
    "ASSIGN",
    "NEGATE",
    "ADD",
    "SUB",
    "MUL",
    "DIV",
    "ADD_ASSIGN",
    "SUB_ASSIGN",
    "MUL_ASSIGN",
    "DIV_ASSIGN",
    "INC",
    "DEC",
    "EQUALS",
    "NOT_EQUALS",
    "LT",
    "GT",
    "LT_EQUALS",
    "GT_EQUALS",
    "OR_BITWISE",
    "OR_LOGICAL",
    "AND_BITWISE",
    "AND_LOGICAL",
    "XOR_BITWISE",
    "NOT_BITWISE",
    "LSHIFT",
    "RSHIFT",
    "QUESTION",
    "COLON",
    "INT",
    "NAME",
    "STRING",
    "CHAR",
    "TYPE_U32",
    "TYPE_S8",
    "KEYWORD_EXT",
    "KEYWORD_RET",
    "KEYWORD_IF",
    "KEYWORD_ELSE",
    "KEYWORD_FOR",
    "KEYWORD_BREAK",
    "KEYWORD_CONTINUE",
    "INVALID"};

typedef struct thrive_token
{

    thrive_token_kind kind;

    s8 *start;
    s8 *end;

    u32 line;
    u32 column;

    union value
    {
        u32 number;
    } value;
} thrive_token;

typedef struct thrive_ast thrive_ast;

typedef struct thrive_state
{
    s8 *source_code;
    u32 source_code_size;

    thrive_token current;

    s8 *line_start;
    u32 line;   /* current line number, start at 1 */
    u32 column; /* current column, start at 1 */

    thrive_ast *ast_pool;
    u32 ast_count;
    u32 ast_capacity;

} thrive_state;

typedef enum thrive_ast_kind
{
    THRIVE_AST_INT,         /* 123 */
    THRIVE_AST_NAME,        /* my_var */
    THRIVE_AST_BINARY,      /* a + b */
    THRIVE_AST_UNARY,       /* -a */
    THRIVE_AST_TERNARY,     /* 1 > a ? 1 : 0 */
    THRIVE_AST_IF,          /* if (cond) { ... } else { ... } */
    THRIVE_AST_FOR,         /* for (init : cond : step) { ... } */
    THRIVE_AST_BREAK,       /* break */
    THRIVE_AST_CONTINUE,    /* continue */
    THRIVE_AST_DEREF,       /* *ptr */
    THRIVE_AST_ADDR_OF,     /* &var */
    THRIVE_AST_RETURN,      /* ret expr */
    THRIVE_AST_ASSIGN,      /* a = b (or a += b) */
    THRIVE_AST_DECL,        /* u32 a = 10 */
    THRIVE_AST_BLOCK,       /* { stmt1 stmt2 } */
    THRIVE_AST_FUNC_DECL,   /* u32 func(u32 a) { ... } */
    THRIVE_AST_FUNC_CALL,   /* func(a : b) */
    THRIVE_AST_EXT_DECL,    /* ext u32 func(u32 a) */
    THRIVE_AST_STRING,      /* "deadbeef" */
    THRIVE_AST_ARRAY_ACCESS /* arr[0] */

} thrive_ast_kind;

struct thrive_ast
{
    thrive_ast_kind kind;
    thrive_ast *next;

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
            thrive_token_kind op;
            thrive_ast *expr;
        } unary;

        struct
        {
            thrive_ast *cond;
            thrive_ast *then_expr;
            thrive_ast *else_expr;
        } ternary;

        struct
        {
            thrive_ast *cond;
            thrive_ast *then_branch;
            thrive_ast *else_branch; /* can be NULL */
        } if_stmt;

        struct
        {
            thrive_ast *init; /* i = 0 */
            thrive_ast *cond; /* i < 10 */
            thrive_ast *step; /* i ++ */
            thrive_ast *body; /* { code } */
        } for_loop;

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
            u8 is_array;    /* 1 if array, 0 if normal var */
            u32 array_size; /* size of the array */
        } decl;

        struct
        {
            thrive_ast *left;  /* the array pointer/name */
            thrive_ast *index; /* the index expression */
        } array_access;

        struct
        {
            thrive_ast *name;
            thrive_ast *params; /* Points to first param node (linked via .next) */
            thrive_ast *body;
        } func_decl;

        struct
        {
            thrive_ast *name;
            thrive_ast *args; /* Points to first argument node (linked via .next) */
        } func_call;

        struct
        {
            thrive_ast *name;
            thrive_ast *params; /* Points to first param node (linked via .next) */
        } ext_decl;

        struct
        {
            s8 *start;
            u32 length;
        } string_lit;

        struct
        {
            thrive_ast *body; /* first statement in the block */
        } block;

    } data;
};

typedef struct thrive_buffer
{
    u8 *data;
    u32 size;
    u32 capacity;

} thrive_buffer;

typedef enum thrive_x64_reg
{
    REG_RAX = 0,
    REG_RCX = 1,
    REG_RDX = 2,
    REG_RBX = 3,
    REG_RSP = 4,
    REG_RBP = 5,
    REG_RSI = 6,
    REG_RDI = 7,
    REG_R8 = 8,
    REG_R9 = 9,
    REG_R10 = 10,
    REG_R11 = 11,
    REG_R12 = 12,
    REG_R13 = 13,
    REG_R14 = 14,
    REG_R15 = 15
} thrive_x64_reg;

typedef enum thrive_x64_op_ext
{
    OP_EXT_ADD = 0,
    OP_EXT_SUB = 5,
    OP_EXT_MOV = 0
} thrive_x64_op_ext;

typedef enum thrive_x64_cc
{
    CC_E = 4,
    CC_NE = 5,
    CC_L = 12,
    CC_LE = 14,
    CC_G = 15,
    CC_GE = 13
} thrive_x64_cc;

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

THRIVE_API THRIVE_INLINE u32 thrive_string_equals(s8 *a, s8 *b, u32 len)
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

THRIVE_API THRIVE_INLINE u32 thrive_align_up(u32 val, u32 align)
{
    if (align == 0)
    {
        return val;
    }

    return (val + align - 1) & ~(align - 1);
}

/* #############################################################################
 * # [SECTION] Thrive Status
 * #############################################################################
 */
THRIVE_API void thrive_panic(thrive_status status);

THRIVE_API void thrive_error(thrive_state *state, thrive_status_type type, s8 *message)
{
    thrive_status status = {0};

    status.type = type;
    status.message = message;
    status.token_start = state->current.start;
    status.token_end = state->current.end;
    status.line = state->current.line;
    status.column = state->current.column;
    status.line_start = state->line_start;

    thrive_panic(status);
}

/* #############################################################################
 * # [SECTION] Lexer
 * #############################################################################
 */
THRIVE_API u32 thrive_token_is_digit_base(s8 c, u32 base)
{
    if (base == 10)
    {
        return c >= '0' && c <= '9';
    }
    else if (base == 16)
    {
        return (c >= '0' && c <= '9') ||
               (c >= 'a' && c <= 'f') ||
               (c >= 'A' && c <= 'F');
    }
    else if (base == 2)
    {
        return c == '0' || c == '1';
    }
    return 0;
}

THRIVE_API i32 thrive_token_digit_value(s8 c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f')
    {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F')
    {
        return 10 + (c - 'A');
    }
    return 0;
}

THRIVE_API THRIVE_INLINE void thrive_token_next(thrive_state *state)
{
    thrive_token token = {0};

repeat:
    token.kind = THRIVE_TOKEN_KIND_INVALID;
    token.start = state->source_code;
    token.line = state->line;
    token.column = state->column;

    if (!*state->source_code)
    {
        token.kind = THRIVE_TOKEN_KIND_EOF;
        token.end = state->source_code;
        state->current = token;
        return;
    }

    /* clang-format off */
    switch (*state->source_code)
    {   
        /* Whitespaces */
        case ' ': case '\r': case '\t': case '\v': case '\f': case '\a':
        {
            do {
                state->source_code++;
                state->column++;
            } while (*state->source_code == ' '  || 
                     *state->source_code == '\r' || 
                     *state->source_code == '\t' ||
                     *state->source_code == '\v' ||
                     *state->source_code == '\f' ||
                     *state->source_code == '\a');

            goto repeat;
        } 
        /* Line comments */
        case ';': 
        {
            while (*state->source_code && *state->source_code != '\n') {
                state->source_code++;
                state->column++;
            }
            
            goto repeat;  
        }
        /* String Literals */
        case '"':
        {
            state->source_code++; state->column++;
            token.start = state->source_code; 
            
            while (*state->source_code && *state->source_code != '"') {

                if (*state->source_code == '\n') 
                {
                    state->line++; state->column = 1; 
                }

                if (*state->source_code == '\\' && *(state->source_code + 1)) 
                {
                    state->source_code++; state->column++;
                }

                state->source_code++; state->column++;
            }
            
            token.kind = THRIVE_TOKEN_KIND_STRING;
            token.end = state->source_code;
            
            if (*state->source_code == '"') {
                state->source_code++; state->column++;
            }

            state->current = token;
            return;
        }
        /* Char Literals */
        case '\'':
        {
            u32 val = 0;
            state->source_code++; state->column++; /* Skip opening ' */
            
            if (*state->source_code == '\\') 
            {
                state->source_code++; 
                state->column++;
                
                switch (*state->source_code) {
                    case 'n': val = '\n'; break;
                    case 'r': val = '\r'; break;
                    case 't': val = '\t'; break;
                    case '0': val = '\0'; break;
                    default:  val = (u32) *state->source_code; break;
                }
            } 
            else 
            {
                val = (u32) *state->source_code;
            }
            
            state->source_code++; state->column++;

            if (*state->source_code == '\'')
            {
                state->source_code++; state->column++; /* Skip closing ' */
            }
            
            token.kind = THRIVE_TOKEN_KIND_CHAR;
            token.value.number = val;

            state->current = token;
            return;
        }
        /* Number processing */
        case '0': case '1': case '2': case '3': case '4': case '5': case '6':
        case '7': case '8': case '9':
        {
            u32 value = 0;
            u32 base = 10;
            i32 seen_digit = 0;

            /* Detect base */
            if (*state->source_code == '0') 
            {
                s8 next = *(state->source_code + 1);

                if (next == 'x' || next == 'X') 
                {
                    base = 16;
                    state->source_code += 2;
                    state->column += 2;
                } 
                else if (next == 'b' || next == 'B') 
                {
                    base = 2;
                    state->source_code += 2;
                    state->column += 2;
                }
            }

            while (1) 
            {
                s8 c = *state->source_code;

                if (c == '_') {
                    state->source_code++;
                    state->column++;
                    continue;
                }

                if (!thrive_token_is_digit_base(c, base)) 
                {
                    break;
                }

                seen_digit = 1;

                value *= base;
                value += (u32)thrive_token_digit_value(c);

                state->source_code++;
                state->column++;
            }

            if (!seen_digit) 
            {
                /* TODO: if no digits after 0x / 0b */
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
            u32 token_length;

            while (thrive_char_is_alpha(*state->source_code) ||
                   thrive_char_is_digit(*state->source_code) ||
                   *state->source_code == '_') 
            {
                state->source_code++;
                state->column++;
            }

            token.kind = THRIVE_TOKEN_KIND_NAME;
            token_length = (u32)(state->source_code - token.start);

            switch (token_length)
            {
                case 2:
                    if (token.start[0] == 'i' && token.start[1] == 'f')
                        token.kind = THRIVE_TOKEN_KIND_KEYWORD_IF;
                    else if (token.start[0] == 's' && token.start[1] == '8')
                        token.kind = THRIVE_TOKEN_KIND_TYPE_S8;
                    break;
                case 3:
                    if (token.start[0] == 'r' && token.start[1] == 'e' && token.start[2] == 't')
                        token.kind = THRIVE_TOKEN_KIND_KEYWORD_RET;
                    else if (token.start[0] == 'u' && token.start[1] == '3' && token.start[2] == '2')
                        token.kind = THRIVE_TOKEN_KIND_TYPE_U32;
                    else if (token.start[0] == 'f' && token.start[1] == 'o' && token.start[2] == 'r')
                        token.kind = THRIVE_TOKEN_KIND_KEYWORD_FOR;
                    else if (token.start[0] == 'e' && token.start[1] == 'x' && token.start[2] == 't')
                        token.kind = THRIVE_TOKEN_KIND_KEYWORD_EXT;
                    break;
                case 4:
                    if (token.start[0] == 'e' && token.start[1] == 'l' && token.start[2] == 's' && token.start[3] == 'e')
                        token.kind = THRIVE_TOKEN_KIND_KEYWORD_ELSE;
                    break;
                case 5:
                    if (token.start[0] == 'b' && token.start[1] == 'r' && token.start[2] == 'e' && token.start[3] == 'a'  && token.start[4] == 'k')
                        token.kind = THRIVE_TOKEN_KIND_KEYWORD_BREAK;
                    break;
                case 8:
                    if (token.start[0] == 'c' && token.start[1] == 'o' && token.start[2] == 'n' && token.start[3] == 't' &&
                        token.start[4] == 'i' && token.start[5] == 'n' && token.start[6] == 'u' && token.start[7] == 'e')
                        token.kind = THRIVE_TOKEN_KIND_KEYWORD_CONTINUE;
                    break;
            }

            break;
        }
        /* New Line handling */
        case '\n': 
        { 
            state->source_code++; 
            state->line++;
            state->column = 1; 
            state->line_start = state->source_code; 
            token.kind = THRIVE_TOKEN_KIND_NEW_LINE; 
            break; 
        }
        /* Single char tokens */
        #define THRIVE_TOKEN_CASE_1(c1, k1) \
            case c1:  {                     \
              state->source_code++;         \
              state->column++;              \
              token.kind = k1;              \
              break; }

        THRIVE_TOKEN_CASE_1('?',  THRIVE_TOKEN_KIND_QUESTION)
        THRIVE_TOKEN_CASE_1(':',  THRIVE_TOKEN_KIND_COLON   )
        THRIVE_TOKEN_CASE_1('(',  THRIVE_TOKEN_KIND_LPAREN  )
        THRIVE_TOKEN_CASE_1(')',  THRIVE_TOKEN_KIND_RPAREN  )
        THRIVE_TOKEN_CASE_1('{',  THRIVE_TOKEN_KIND_LBRACE  )
        THRIVE_TOKEN_CASE_1('}',  THRIVE_TOKEN_KIND_RBRACE  )
        THRIVE_TOKEN_CASE_1('[',  THRIVE_TOKEN_KIND_LBRACKET)
        THRIVE_TOKEN_CASE_1(']',  THRIVE_TOKEN_KIND_RBRACKET)
        THRIVE_TOKEN_CASE_1('\0', THRIVE_TOKEN_KIND_EOF     )

        #undef THRIVE_TOKEN_CASE_1

        #define THRIVE_TOKEN_CASE_2(c1, k1, c2, k2) \
            case c1:  {                     \
              state->source_code++;         \
              state->column++;              \
              token.kind = k1;              \
              if (*state->source_code == c2) { \
                 token.kind = k2;              \
                 state->source_code++;         \
                 state->column++;              \
              }                                \
              break; }

        THRIVE_TOKEN_CASE_2('=', THRIVE_TOKEN_KIND_ASSIGN, '=', THRIVE_TOKEN_KIND_EQUALS)
        THRIVE_TOKEN_CASE_2('!', THRIVE_TOKEN_KIND_NEGATE, '=', THRIVE_TOKEN_KIND_NOT_EQUALS)
        THRIVE_TOKEN_CASE_2('*', THRIVE_TOKEN_KIND_MUL, '=', THRIVE_TOKEN_KIND_MUL_ASSIGN)
        THRIVE_TOKEN_CASE_2('/', THRIVE_TOKEN_KIND_DIV, '=', THRIVE_TOKEN_KIND_DIV_ASSIGN)
        THRIVE_TOKEN_CASE_2('|', THRIVE_TOKEN_KIND_OR_BITWISE, '|', THRIVE_TOKEN_KIND_OR_LOGICAL)     
        THRIVE_TOKEN_CASE_2('&', THRIVE_TOKEN_KIND_AND_BITWISE, '&', THRIVE_TOKEN_KIND_AND_LOGICAL)     

        #undef THRIVE_TOKEN_CASE_2

        #define THRIVE_TOKEN_CASE_3(c1, k1, c2, k2, c3, k3) \
            case c1:  {                     \
              state->source_code++;         \
              state->column++;              \
              token.kind = k1;              \
              if (*state->source_code == c2) { \
                 token.kind = k2;              \
                 state->source_code++;         \
                 state->column++;              \
              } else if (*state->source_code == c3) { \
                 token.kind = k3;                     \
                 state->source_code++;                \
                 state->column++;                     \
              }                                       \
              break; }

        THRIVE_TOKEN_CASE_3('<', THRIVE_TOKEN_KIND_LT, '<', THRIVE_TOKEN_KIND_LSHIFT, '=', THRIVE_TOKEN_KIND_LT_EQUALS)        
        THRIVE_TOKEN_CASE_3('>', THRIVE_TOKEN_KIND_GT, '>', THRIVE_TOKEN_KIND_RSHIFT, '=', THRIVE_TOKEN_KIND_GT_EQUALS)      
        THRIVE_TOKEN_CASE_3('+', THRIVE_TOKEN_KIND_ADD, '+', THRIVE_TOKEN_KIND_INC, '=', THRIVE_TOKEN_KIND_ADD_ASSIGN)
        THRIVE_TOKEN_CASE_3('-', THRIVE_TOKEN_KIND_SUB, '-', THRIVE_TOKEN_KIND_DEC, '=', THRIVE_TOKEN_KIND_SUB_ASSIGN)                

        #undef THRIVE_TOKEN_CASE_3

        default:  
        { 
            state->source_code++; 
            state->column++; 
            token.kind = THRIVE_TOKEN_KIND_INVALID;  
            break; 
        }
    }
    /* clang-format on */

    token.end = state->source_code;

    state->current = token;
}

THRIVE_API u8 thrive_token_accept(thrive_state *state, thrive_token_kind kind)
{
    if (state->current.kind == kind)
    {
        thrive_token_next(state);
        return 1;
    }
    return 0;
}

THRIVE_API void thrive_token_expect(thrive_state *state, thrive_token_kind kind)
{
    if (state->current.kind != kind)
    {
        thrive_error(state, THRIVE_STATUS_ERROR_SYNTAX, "Unexpected token. Expected a different token kind.");
    }

    thrive_token_next(state);
}

THRIVE_API u8 thrive_token_accept_type(thrive_state *state)
{
    if (state->current.kind == THRIVE_TOKEN_KIND_TYPE_U32 ||
        state->current.kind == THRIVE_TOKEN_KIND_TYPE_S8)
    {
        thrive_token_next(state);
        return 1;
    }
    return 0;
}

THRIVE_API void thrive_token_expect_type(thrive_state *state)
{
    if (state->current.kind == THRIVE_TOKEN_KIND_TYPE_U32 ||
        state->current.kind == THRIVE_TOKEN_KIND_TYPE_S8)
    {
        thrive_token_next(state);
    }
    else
    {
        thrive_error(state, THRIVE_STATUS_ERROR_SYNTAX, "Expected type identifier");
    }
}

THRIVE_API void thrive_token_skip_newlines(thrive_state *state)
{
    while (state->current.kind == THRIVE_TOKEN_KIND_NEW_LINE)
    {
        thrive_token_next(state);
    }
}

/* #############################################################################
 * # [SECTION] AST Parser
 * #############################################################################
 */
THRIVE_API THRIVE_INLINE thrive_ast *thrive_ast_create(thrive_state *state, thrive_ast_kind kind)
{
    thrive_ast *node;

    if (state->ast_count >= state->ast_capacity)
    {
        thrive_error(state, THRIVE_STATUS_ERROR_MEMORY, "AST pool exhausted");
    }

    node = &state->ast_pool[state->ast_count++];
    node->kind = kind;

    return node;
}

/*
primary
→ unary
→ mul
→ add
→ comparison
→ equality
→ bitwise AND (&)
→ bitwise OR (|)
→ logical AND (&&)
→ logical OR (||)
→ ternary
→ assign
→ expr
*/
THRIVE_API thrive_ast *thrive_ast_parse_expression(thrive_state *state);

THRIVE_API thrive_ast *thrive_ast_parse_primary(thrive_state *state)
{
    thrive_token tok = state->current;

    if (tok.kind == THRIVE_TOKEN_KIND_INVALID)
    {
        thrive_error(state, THRIVE_STATUS_ERROR_SYNTAX, "Invalid token");
        return 0;
    }

    if (tok.kind == THRIVE_TOKEN_KIND_INT)
    {
        thrive_ast *node = thrive_ast_create(state, THRIVE_AST_INT);
        node->data.int_value = tok.value.number;
        thrive_token_next(state);
        return node;
    }

    if (tok.kind == THRIVE_TOKEN_KIND_NAME)
    {
        thrive_ast *node = thrive_ast_create(state, THRIVE_AST_NAME);
        node->data.name.start = tok.start;
        node->data.name.length = (u32)(tok.end - tok.start);
        thrive_token_next(state);
        return node;
    }

    if (thrive_token_accept(state, THRIVE_TOKEN_KIND_LPAREN))
    {
        thrive_ast *expr = thrive_ast_parse_expression(state);

        thrive_token_expect(state, THRIVE_TOKEN_KIND_RPAREN);

        return expr;
    }

    if (tok.kind == THRIVE_TOKEN_KIND_CHAR)
    {
        thrive_ast *node = thrive_ast_create(state, THRIVE_AST_INT);
        node->data.int_value = tok.value.number;
        thrive_token_next(state);
        return node;
    }

    if (tok.kind == THRIVE_TOKEN_KIND_STRING)
    {
        thrive_ast *node = thrive_ast_create(state, THRIVE_AST_STRING);
        node->data.string_lit.start = tok.start;
        node->data.string_lit.length = (u32)(tok.end - tok.start);
        thrive_token_next(state);
        return node;
    }

    thrive_error(state, THRIVE_STATUS_ERROR_SYNTAX, "Expected primary expression");

    return 0;
}

/* Prefix binding powers (e.g., -a, !a) */
THRIVE_API THRIVE_INLINE i32 thrive_ast_prefix_bp(thrive_token_kind kind, i32 *r_bp)
{
    switch (kind)
    {
    case THRIVE_TOKEN_KIND_SUB:         /* -i  */
    case THRIVE_TOKEN_KIND_ADD:         /* +i  */
    case THRIVE_TOKEN_KIND_NEGATE:      /* !i  */
    case THRIVE_TOKEN_KIND_INC:         /* ++i */
    case THRIVE_TOKEN_KIND_DEC:         /* --i */
    case THRIVE_TOKEN_KIND_MUL:         /* *ptr */
    case THRIVE_TOKEN_KIND_AND_BITWISE: /* &var */
        *r_bp = 110;                    /* High priority */
        return 1;
    default:
        return 0;
    }
}

/* Infix binding powers (e.g., a + b, a = b) */
THRIVE_API THRIVE_INLINE i32 thrive_ast_infix_bp(thrive_token_kind op, i32 *l_bp, i32 *r_bp)
{
    switch (op)
    {
    /* Assignment: Right Associative */
    case THRIVE_TOKEN_KIND_ASSIGN:
    case THRIVE_TOKEN_KIND_ADD_ASSIGN:
    case THRIVE_TOKEN_KIND_SUB_ASSIGN:
    case THRIVE_TOKEN_KIND_MUL_ASSIGN:
    case THRIVE_TOKEN_KIND_DIV_ASSIGN:
        *l_bp = 10;
        *r_bp = 9;
        return 1;

    /* Ternary: Right Associative (handled specifically in parser) */
    case THRIVE_TOKEN_KIND_QUESTION:
        *l_bp = 20;
        *r_bp = 19;
        return 1;

    /* Logical OR */
    case THRIVE_TOKEN_KIND_OR_LOGICAL:
        *l_bp = 30;
        *r_bp = 31;
        return 1;

    /* Logical AND */
    case THRIVE_TOKEN_KIND_AND_LOGICAL:
        *l_bp = 40;
        *r_bp = 41;
        return 1;

    /* Bitwise OR */
    case THRIVE_TOKEN_KIND_OR_BITWISE:
        *l_bp = 50;
        *r_bp = 51;
        return 1;

    /* Bitwise AND */
    case THRIVE_TOKEN_KIND_AND_BITWISE:
        *l_bp = 60;
        *r_bp = 61;
        return 1;

    /* Equality */
    case THRIVE_TOKEN_KIND_EQUALS:
    case THRIVE_TOKEN_KIND_NOT_EQUALS:
        *l_bp = 70;
        *r_bp = 71;
        return 1;

    /* Comparison */
    case THRIVE_TOKEN_KIND_LT:
    case THRIVE_TOKEN_KIND_GT:
    case THRIVE_TOKEN_KIND_LT_EQUALS:
    case THRIVE_TOKEN_KIND_GT_EQUALS:
        *l_bp = 80;
        *r_bp = 81;
        return 1;

    /* Bitwise Shifts */
    case THRIVE_TOKEN_KIND_LSHIFT:
    case THRIVE_TOKEN_KIND_RSHIFT:
        *l_bp = 85;
        *r_bp = 86;
        return 1;

    /* Addition / Subtraction */
    case THRIVE_TOKEN_KIND_ADD:
    case THRIVE_TOKEN_KIND_SUB:
        *l_bp = 90;
        *r_bp = 91;
        return 1;

    /* Multiplication / Division */
    case THRIVE_TOKEN_KIND_MUL:
    case THRIVE_TOKEN_KIND_DIV:
        *l_bp = 100;
        *r_bp = 101;
        return 1;

    case THRIVE_TOKEN_KIND_INC:
    case THRIVE_TOKEN_KIND_DEC:
        *l_bp = 120;
        *r_bp = 121;
        return 1;

    /* Postfix / Call: Highest priority */
    case THRIVE_TOKEN_KIND_LPAREN:
    case THRIVE_TOKEN_KIND_LBRACKET:
        *l_bp = 150;
        *r_bp = 151;
        return 1;

    default:
        return 0;
    }
}

THRIVE_API thrive_ast *thrive_ast_parse_expression_bp(thrive_state *state, i32 min_bp)
{
    thrive_ast *left = 0;
    i32 p_rbp;

    if (thrive_ast_prefix_bp(state->current.kind, &p_rbp))
    {
        thrive_token_kind op = state->current.kind;
        thrive_token_next(state);
        left = thrive_ast_create(state, THRIVE_AST_UNARY);

        switch (op)
        {
        case THRIVE_TOKEN_KIND_MUL:
            left->kind = THRIVE_AST_DEREF;
            break;
        case THRIVE_TOKEN_KIND_AND_BITWISE:
            left->kind = THRIVE_AST_ADDR_OF;
            break;
        default:
            left->data.unary.op = op;
            break;
        }
        left->data.unary.expr = thrive_ast_parse_expression_bp(state, p_rbp);
    }
    else
    {
        left = thrive_ast_parse_primary(state);
    }

    while (1)
    {
        thrive_token_kind op = state->current.kind;
        i32 l_bp;
        i32 r_bp;

        if (!thrive_ast_infix_bp(op, &l_bp, &r_bp) || l_bp < min_bp)
        {
            break;
        }

        thrive_token_next(state);

        if (op == THRIVE_TOKEN_KIND_LPAREN)
        {
            thrive_ast *call_node = thrive_ast_create(state, THRIVE_AST_FUNC_CALL);
            thrive_ast **tail;

            call_node->data.func_call.name = left;
            tail = &call_node->data.func_call.args;

            while (state->current.kind != THRIVE_TOKEN_KIND_RPAREN)
            {
                thrive_ast *arg = thrive_ast_parse_expression(state);
                *tail = arg;
                tail = &arg->next;

                if (!thrive_token_accept(state, THRIVE_TOKEN_KIND_COLON))
                {
                    break;
                }
            }

            thrive_token_expect(state, THRIVE_TOKEN_KIND_RPAREN);

            left = call_node;
        }
        else if (op == THRIVE_TOKEN_KIND_LBRACKET)
        {
            thrive_ast *index_expr = thrive_ast_parse_expression(state);
            thrive_ast *node;
            thrive_token_expect(state, THRIVE_TOKEN_KIND_RBRACKET);

            node = thrive_ast_create(state, THRIVE_AST_ARRAY_ACCESS);
            node->data.array_access.left = left;
            node->data.array_access.index = index_expr;

            left = node;
        }
        else if (op == THRIVE_TOKEN_KIND_INC || op == THRIVE_TOKEN_KIND_DEC)
        {
            thrive_ast *node = thrive_ast_create(state, THRIVE_AST_UNARY);
            node->data.unary.op = op;
            node->data.unary.expr = left;

            left = node;
        }
        else if (op == THRIVE_TOKEN_KIND_QUESTION)
        {
            thrive_ast *then_expr = thrive_ast_parse_expression_bp(state, 0);
            thrive_ast *else_expr;
            thrive_ast *node;

            thrive_token_expect(state, THRIVE_TOKEN_KIND_COLON);

            else_expr = thrive_ast_parse_expression_bp(state, r_bp);

            node = thrive_ast_create(state, THRIVE_AST_TERNARY);
            node->data.ternary.cond = left;
            node->data.ternary.then_expr = then_expr;
            node->data.ternary.else_expr = else_expr;

            left = node;
        }
        else if (op == THRIVE_TOKEN_KIND_ASSIGN ||
                 op == THRIVE_TOKEN_KIND_ADD_ASSIGN ||
                 op == THRIVE_TOKEN_KIND_SUB_ASSIGN ||
                 op == THRIVE_TOKEN_KIND_MUL_ASSIGN ||
                 op == THRIVE_TOKEN_KIND_DIV_ASSIGN)
        {
            thrive_ast *right = thrive_ast_parse_expression_bp(state, r_bp);

            if (op == THRIVE_TOKEN_KIND_ASSIGN)
            {
                thrive_ast *node = thrive_ast_create(state, THRIVE_AST_ASSIGN);
                node->data.assign.left = left;
                node->data.assign.right = right;

                left = node;
            }
            else
            {
                thrive_ast *binary = thrive_ast_create(state, THRIVE_AST_BINARY);
                thrive_ast *assign = thrive_ast_create(state, THRIVE_AST_ASSIGN);

                binary->data.binary.left = left;
                binary->data.binary.right = right;

                if (op == THRIVE_TOKEN_KIND_ADD_ASSIGN)
                {
                    binary->data.binary.op = THRIVE_TOKEN_KIND_ADD;
                }
                else if (op == THRIVE_TOKEN_KIND_SUB_ASSIGN)
                {
                    binary->data.binary.op = THRIVE_TOKEN_KIND_SUB;
                }
                else if (op == THRIVE_TOKEN_KIND_MUL_ASSIGN)
                {
                    binary->data.binary.op = THRIVE_TOKEN_KIND_MUL;
                }
                else if (op == THRIVE_TOKEN_KIND_DIV_ASSIGN)
                {
                    binary->data.binary.op = THRIVE_TOKEN_KIND_DIV;
                }

                assign->data.assign.left = left;
                assign->data.assign.right = binary;

                left = assign;
            }
        }
        else
        {
            thrive_ast *right = thrive_ast_parse_expression_bp(state, r_bp);

            thrive_ast *node = thrive_ast_create(state, THRIVE_AST_BINARY);
            node->data.binary.op = op;
            node->data.binary.left = left;
            node->data.binary.right = right;

            left = node;
        }
    }

    return left;
}

THRIVE_API thrive_ast *thrive_ast_parse_expression(thrive_state *state)
{
    return thrive_ast_parse_expression_bp(state, 0);
}

THRIVE_API thrive_ast *thrive_ast_parse_statement(thrive_state *state);

THRIVE_API thrive_ast *thrive_ast_parse_block_statement(thrive_state *state)
{
    thrive_ast *block_node = thrive_ast_create(state, THRIVE_AST_BLOCK);
    thrive_ast **tail = &block_node->data.block.body;

    thrive_token_expect(state, THRIVE_TOKEN_KIND_LBRACE);
    thrive_token_skip_newlines(state);

    while (state->current.kind != THRIVE_TOKEN_KIND_RBRACE &&
           state->current.kind != THRIVE_TOKEN_KIND_EOF)
    {
        thrive_ast *stmt = thrive_ast_parse_statement(state);

        if (!stmt)
        {
            return 0;
        }

        *tail = stmt;       /* Attach new stmt to the end of the list */
        tail = &stmt->next; /* Move tail pointer to the new end */

        thrive_token_skip_newlines(state);
    }

    thrive_token_expect(state, THRIVE_TOKEN_KIND_RBRACE);

    return block_node;
}

THRIVE_API thrive_ast *thrive_ast_parse_statement(thrive_state *state)
{
    if (state->current.kind == THRIVE_TOKEN_KIND_LBRACE)
    {
        return thrive_ast_parse_block_statement(state);
    }

    /* ext u32 Name(u32 a : u32 b) */
    if (thrive_token_accept(state, THRIVE_TOKEN_KIND_KEYWORD_EXT))
    {
        thrive_ast *node = thrive_ast_create(state, THRIVE_AST_EXT_DECL);
        thrive_token name_tok;
        thrive_ast **p_tail;

        node->data.ext_decl.params = 0;
        p_tail = &node->data.ext_decl.params;

        /* Skip return type for now */
        thrive_token_expect_type(state);

        name_tok = state->current;
        thrive_token_expect(state, THRIVE_TOKEN_KIND_NAME);

        node->data.ext_decl.name = thrive_ast_create(state, THRIVE_AST_NAME);
        node->data.ext_decl.name->data.name.start = name_tok.start;
        node->data.ext_decl.name->data.name.length = (u32)(name_tok.end - name_tok.start);

        thrive_token_expect(state, THRIVE_TOKEN_KIND_LPAREN);

        while (state->current.kind != THRIVE_TOKEN_KIND_RPAREN &&
               state->current.kind != THRIVE_TOKEN_KIND_EOF)
        {
            thrive_token p_tok;
            thrive_ast *p_node;

            /* Skip parameter type for now */
            thrive_token_expect_type(state);

            /* Handle pointer '*' if present */
            thrive_token_accept(state, THRIVE_TOKEN_KIND_MUL);

            p_tok = state->current;
            thrive_token_expect(state, THRIVE_TOKEN_KIND_NAME);

            p_node = thrive_ast_create(state, THRIVE_AST_NAME);
            p_node->data.name.start = p_tok.start;
            p_node->data.name.length = (u32)(p_tok.end - p_tok.start);

            *p_tail = p_node;
            p_tail = &p_node->next;

            if (!thrive_token_accept(state, THRIVE_TOKEN_KIND_COLON))
            {
                break;
            }
        }

        thrive_token_expect(state, THRIVE_TOKEN_KIND_RPAREN);

        return node;
    }

    /* return */
    if (thrive_token_accept(state, THRIVE_TOKEN_KIND_KEYWORD_RET))
    {
        thrive_ast *node = thrive_ast_create(state, THRIVE_AST_RETURN);
        node->data.ret.expr = thrive_ast_parse_expression(state);
        return node;
    }

    /* declaration: u32 a = expr OR u32 func(u32 a : u32 b) */
    if (thrive_token_accept_type(state))
    {
        thrive_token name_tok;
        thrive_ast *name;
        thrive_ast *node;
        u8 is_pointer = 0;

        if (thrive_token_accept(state, THRIVE_TOKEN_KIND_MUL))
        {
            is_pointer = 1;
        }
        (void)is_pointer;

        name_tok = state->current;

        thrive_token_expect(state, THRIVE_TOKEN_KIND_NAME);

        name = thrive_ast_create(state, THRIVE_AST_NAME);
        name->data.name.start = name_tok.start;
        name->data.name.length = (u32)(name_tok.end - name_tok.start);

        /* function declaration */
        if (thrive_token_accept(state, THRIVE_TOKEN_KIND_LPAREN))
        {
            thrive_ast **p_tail;
            node = thrive_ast_create(state, THRIVE_AST_FUNC_DECL);
            node->data.func_decl.name = name;
            node->data.func_decl.params = 0;

            p_tail = &node->data.func_decl.params;

            while (state->current.kind != THRIVE_TOKEN_KIND_RPAREN &&
                   state->current.kind != THRIVE_TOKEN_KIND_EOF)
            {
                if (thrive_token_accept_type(state))
                {
                    thrive_token p_tok;
                    thrive_ast *p_name;

                    thrive_token_accept(state, THRIVE_TOKEN_KIND_MUL);

                    p_tok = state->current;
                    thrive_token_expect(state, THRIVE_TOKEN_KIND_NAME);

                    p_name = thrive_ast_create(state, THRIVE_AST_NAME);
                    p_name->data.name.start = p_tok.start;
                    p_name->data.name.length = (u32)(p_tok.end - p_tok.start);

                    *p_tail = p_name;
                    p_tail = &p_name->next;
                }

                if (!thrive_token_accept(state, THRIVE_TOKEN_KIND_COLON))
                {
                    break;
                }
            }

            thrive_token_expect(state, THRIVE_TOKEN_KIND_RPAREN);
            thrive_token_skip_newlines(state);

            node->data.func_decl.body = thrive_ast_parse_block_statement(state);
            return node;
        }

        /* Normal Variable Declaration */
        node = thrive_ast_create(state, THRIVE_AST_DECL);
        node->data.decl.name = name;
        node->data.decl.value = 0;
        node->data.decl.is_array = 0;
        node->data.decl.array_size = 0;

        if (thrive_token_accept(state, THRIVE_TOKEN_KIND_LBRACKET))
        {
            node->data.decl.is_array = 1;

            if (state->current.kind == THRIVE_TOKEN_KIND_INT)
            {
                node->data.decl.array_size = state->current.value.number;
                thrive_token_next(state);
            }
            else
            {
                thrive_error(state, THRIVE_STATUS_ERROR_SYNTAX, "Expected integer literal for array size");
            }

            thrive_token_expect(state, THRIVE_TOKEN_KIND_RBRACKET);
        }
        else if (thrive_token_accept(state, THRIVE_TOKEN_KIND_ASSIGN))
        {
            node->data.decl.value = thrive_ast_parse_expression(state);
        }

        return node;
    }

    if (thrive_token_accept(state, THRIVE_TOKEN_KIND_KEYWORD_IF))
    {
        thrive_ast *node = thrive_ast_create(state, THRIVE_AST_IF);

        thrive_token_expect(state, THRIVE_TOKEN_KIND_LPAREN);

        node->data.if_stmt.cond = thrive_ast_parse_expression(state);

        thrive_token_expect(state, THRIVE_TOKEN_KIND_RPAREN);

        thrive_token_skip_newlines(state);

        if (state->current.kind == THRIVE_TOKEN_KIND_LBRACE)
        {
            node->data.if_stmt.then_branch = thrive_ast_parse_block_statement(state);
        }
        else
        {
            node->data.if_stmt.then_branch = thrive_ast_parse_statement(state);
        }

        node->data.if_stmt.else_branch = 0;

        thrive_token_skip_newlines(state);

        if (thrive_token_accept(state, THRIVE_TOKEN_KIND_KEYWORD_ELSE))
        {
            thrive_token_skip_newlines(state);

            if (state->current.kind == THRIVE_TOKEN_KIND_LBRACE)
            {
                node->data.if_stmt.else_branch = thrive_ast_parse_block_statement(state);
            }
            else
            {
                node->data.if_stmt.else_branch = thrive_ast_parse_statement(state);
            }
        }

        return node;
    }

    if (thrive_token_accept(state, THRIVE_TOKEN_KIND_KEYWORD_BREAK))
    {
        return thrive_ast_create(state, THRIVE_AST_BREAK);
    }

    if (thrive_token_accept(state, THRIVE_TOKEN_KIND_KEYWORD_CONTINUE))
    {
        return thrive_ast_create(state, THRIVE_AST_CONTINUE);
    }

    if (thrive_token_accept(state, THRIVE_TOKEN_KIND_KEYWORD_FOR))
    {
        thrive_ast *node = thrive_ast_create(state, THRIVE_AST_FOR);

        thrive_token_expect(state, THRIVE_TOKEN_KIND_LPAREN);

        /* 1. Initialization (e.g., i = 0) */
        node->data.for_loop.init = thrive_ast_parse_expression(state);

        thrive_token_expect(state, THRIVE_TOKEN_KIND_COLON);

        /* 2. Condition (e.g., i < 10) */
        node->data.for_loop.cond = thrive_ast_parse_expression(state);

        thrive_token_expect(state, THRIVE_TOKEN_KIND_COLON);

        /* 3. Step/Increment (e.g., i ++) */
        node->data.for_loop.step = thrive_ast_parse_expression(state);

        thrive_token_expect(state, THRIVE_TOKEN_KIND_RPAREN);

        thrive_token_skip_newlines(state);

        /* 4. Body */
        if (state->current.kind == THRIVE_TOKEN_KIND_LBRACE)
        {
            node->data.for_loop.body = thrive_ast_parse_block_statement(state);
        }
        else
        {
            node->data.for_loop.body = thrive_ast_parse_statement(state);
        }

        return node;
    }

    {
        thrive_ast *expr = thrive_ast_parse_expression(state);
        thrive_token_skip_newlines(state);
        return expr;
    }
}

THRIVE_API thrive_ast *thrive_ast_parse(thrive_state *state)
{
    thrive_ast *node;
    thrive_ast **tail;

    state->ast_count = 0;

    node = thrive_ast_create(state, THRIVE_AST_BLOCK);
    tail = &node->data.block.body;

    thrive_token_next(state);
    thrive_token_skip_newlines(state);

    while (state->current.kind != THRIVE_TOKEN_KIND_EOF)
    {
        thrive_ast *stmt;

        thrive_token_skip_newlines(state);

        stmt = thrive_ast_parse_statement(state);

        if (!stmt)
        {
            break;
        }

        /* Append to the global block list */
        *tail = stmt;
        tail = &stmt->next;

        thrive_token_skip_newlines(state);
    }

    return node;
}

THRIVE_API thrive_ast *thrive_ast_fold(thrive_ast *node)
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

        /* Optimization: x * 0 = 0, x & 0 = 0 */
        if ((r && r->kind == THRIVE_AST_INT && r->data.int_value == 0) ||
            (l && l->kind == THRIVE_AST_INT && l->data.int_value == 0))
        {
            if (node->data.binary.op == THRIVE_TOKEN_KIND_MUL ||
                node->data.binary.op == THRIVE_TOKEN_KIND_AND_BITWISE)
            {
                node->kind = THRIVE_AST_INT;
                node->data.int_value = 0;
                return node;
            }
        }

        /* Optimization: x + 0 = x, x - 0 = x, x * 1 = x */
        if (r && r->kind == THRIVE_AST_INT && r->data.int_value == 0)
        {
            if (node->data.binary.op == THRIVE_TOKEN_KIND_ADD ||
                node->data.binary.op == THRIVE_TOKEN_KIND_SUB)
            {
                return l;
            }
        }

        if (r && r->kind == THRIVE_AST_INT && r->data.int_value == 1)
        {
            if (node->data.binary.op == THRIVE_TOKEN_KIND_MUL)
            {
                return l;
            }
        }

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
                {
                    result = a / b;
                }
                else
                {
                    return node;
                }
                break;
            case THRIVE_TOKEN_KIND_EQUALS:
                result = (a == b);
                break;
            case THRIVE_TOKEN_KIND_NOT_EQUALS:
                result = (a != b);
                break;
            case THRIVE_TOKEN_KIND_LT:
                result = (a < b);
                break;
            case THRIVE_TOKEN_KIND_GT:
                result = (a > b);
                break;
            case THRIVE_TOKEN_KIND_LT_EQUALS:
                result = (a <= b);
                break;
            case THRIVE_TOKEN_KIND_GT_EQUALS:
                result = (a >= b);
                break;
            case THRIVE_TOKEN_KIND_AND_BITWISE:
                result = a & b;
                break;
            case THRIVE_TOKEN_KIND_OR_BITWISE:
                result = a | b;
                break;
            case THRIVE_TOKEN_KIND_LSHIFT:
                result = a << b;
                break;
            case THRIVE_TOKEN_KIND_RSHIFT:
                result = a >> b;
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

    case THRIVE_AST_UNARY:
        node->data.unary.expr = thrive_ast_fold(node->data.unary.expr);
        if (node->data.unary.expr->kind == THRIVE_AST_INT)
        {
            u32 val = node->data.unary.expr->data.int_value;
            if (node->data.unary.op == THRIVE_TOKEN_KIND_SUB)
            {
                node->kind = THRIVE_AST_INT;
                node->data.int_value = (u32)(-(i32)val);
                return node;
            }
            if (node->data.unary.op == THRIVE_TOKEN_KIND_NEGATE)
            {
                node->kind = THRIVE_AST_INT;
                node->data.int_value = (val == 0) ? 1 : 0;
                return node;
            }
        }
        return node;

    case THRIVE_AST_ASSIGN:
        node->data.assign.right = thrive_ast_fold(node->data.assign.right);
        return node;

    case THRIVE_AST_IF:
    {
        node->data.if_stmt.cond = thrive_ast_fold(node->data.if_stmt.cond);

        if (node->data.if_stmt.cond->kind == THRIVE_AST_INT)
        {
            if (node->data.if_stmt.cond->data.int_value != 0)
            {
                return thrive_ast_fold(node->data.if_stmt.then_branch);
            }
            else
            {
                return thrive_ast_fold(node->data.if_stmt.else_branch);
            }
        }

        node->data.if_stmt.then_branch = thrive_ast_fold(node->data.if_stmt.then_branch);
        node->data.if_stmt.else_branch = thrive_ast_fold(node->data.if_stmt.else_branch);

        return node;
    }

    case THRIVE_AST_TERNARY:
    {
        node->data.ternary.cond = thrive_ast_fold(node->data.ternary.cond);

        if (node->data.ternary.cond->kind == THRIVE_AST_INT)
        {
            if (node->data.ternary.cond->data.int_value != 0)
            {
                return thrive_ast_fold(node->data.ternary.then_expr);
            }
            else
            {
                return thrive_ast_fold(node->data.ternary.else_expr);
            }
        }

        node->data.ternary.then_expr = thrive_ast_fold(node->data.ternary.then_expr);
        node->data.ternary.else_expr = thrive_ast_fold(node->data.ternary.else_expr);

        return node;
    }

    case THRIVE_AST_DECL:
        if (node->data.decl.value)
        {
            node->data.decl.value = thrive_ast_fold(node->data.decl.value);
        }
        return node;

    case THRIVE_AST_RETURN:
        node->data.ret.expr = thrive_ast_fold(node->data.ret.expr);
        return node;

    case THRIVE_AST_FOR:
        if (node->data.for_loop.init)
        {
            node->data.for_loop.init = thrive_ast_fold(node->data.for_loop.init);
        }
        if (node->data.for_loop.cond)
        {
            node->data.for_loop.cond = thrive_ast_fold(node->data.for_loop.cond);
        }
        if (node->data.for_loop.step)
        {
            node->data.for_loop.step = thrive_ast_fold(node->data.for_loop.step);
        }
        if (node->data.for_loop.body)
        {
            node->data.for_loop.body = thrive_ast_fold(node->data.for_loop.body);
        }
        return node;

    case THRIVE_AST_BLOCK:
    {
        thrive_ast *curr = node->data.block.body;

        while (curr)
        {
            curr = thrive_ast_fold(curr);
            curr = curr->next;
        }
        return node;
    }

    case THRIVE_AST_FUNC_DECL:
        if (node->data.func_decl.body)
        {
            node->data.func_decl.body = thrive_ast_fold(node->data.func_decl.body);
        }
        return node;

    case THRIVE_AST_FUNC_CALL:
    {
        thrive_ast **curr = &node->data.func_call.args;
        while (*curr)
        {
            *curr = thrive_ast_fold(*curr);
            curr = &(*curr)->next;
        }
        return node;
    }

    case THRIVE_AST_EXT_DECL:
    {
        thrive_ast **curr = &node->data.ext_decl.params;
        while (*curr)
        {
            *curr = thrive_ast_fold(*curr);
            curr = &(*curr)->next;
        }
        return node;
    }

    default:
        return node;
    }
}

/* #############################################################################
 * # [SECTION] Buffered Writer
 * #############################################################################
 */
THRIVE_API THRIVE_INLINE void thrive_buffer_write_u8(thrive_buffer *b, u8 value)
{
    b->data[b->size++] = value;
}

THRIVE_API THRIVE_INLINE void thrive_buffer_write_u16(thrive_buffer *b, u16 value)
{
    thrive_buffer_write_u8(b, (u8)(value & 0xFF));
    thrive_buffer_write_u8(b, (u8)((value >> 8) & 0xFF));
}

THRIVE_API THRIVE_INLINE void thrive_buffer_write_u32(thrive_buffer *b, u32 value)
{
    thrive_buffer_write_u8(b, (u8)(value & 0xFF));
    thrive_buffer_write_u8(b, (u8)((value >> 8) & 0xFF));
    thrive_buffer_write_u8(b, (u8)((value >> 16) & 0xFF));
    thrive_buffer_write_u8(b, (u8)((value >> 24) & 0xFF));
}

THRIVE_API THRIVE_INLINE void thrive_buffer_write_u64(thrive_buffer *b, u64 value)
{
    thrive_buffer_write_u8(b, (u8)(value & 0xFF));
    thrive_buffer_write_u8(b, (u8)((value >> 8) & 0xFF));
    thrive_buffer_write_u8(b, (u8)((value >> 16) & 0xFF));
    thrive_buffer_write_u8(b, (u8)((value >> 24) & 0xFF));
    thrive_buffer_write_u8(b, (u8)((value >> 32) & 0xFF));
    thrive_buffer_write_u8(b, (u8)((value >> 40) & 0xFF));
    thrive_buffer_write_u8(b, (u8)((value >> 48) & 0xFF));
    thrive_buffer_write_u8(b, (u8)((value >> 56) & 0xFF));
}

THRIVE_API THRIVE_INLINE void thrive_buffer_write_bytes(thrive_buffer *b, u8 *data, u32 size)
{
    u32 i;

    for (i = 0; i < size; ++i)
    {
        thrive_buffer_write_u8(b, data[i]);
    }
}

THRIVE_API THRIVE_INLINE void thrive_buffer_align(thrive_buffer *b, u32 align)
{
    while (b->size % align)
    {
        thrive_buffer_write_u8(b, 0);
    }
}

/* #############################################################################
 * # [SECTION] PE32+ Generator
 * #############################################################################
 */
typedef struct thrive_p32_plus_import
{
    s8 *library;       /* e.g. "kernel32.dll" */
    s8 **imports;      /* e.g. "ExitProcess", "Sleep" */
    u32 imports_count; /* count of imports entries */

} thrive_p32_plus_import;

u32 thrive_pe32_plus_get_iat_rva(thrive_p32_plus_import *imports, u32 num_imports, u32 text_vsize, u32 dll_index, u32 func_index)
{
    u32 text_rva = 0x1000;
    u32 rdata_rva = text_rva + thrive_align_up(text_vsize, 0x1000);
    u32 idt_size = (num_imports + 1) * 20;

    u32 num_funcs = 0;
    u32 i;

    u32 ilt_size;
    u32 iat_base_rva;
    u32 offset;

    for (i = 0; i < num_imports; ++i)
    {
        num_funcs += imports[i].imports_count;
    }

    ilt_size = (num_funcs + num_imports) * 8;
    iat_base_rva = rdata_rva + idt_size + ilt_size;

    offset = 0;

    for (i = 0; i < dll_index; ++i)
    {
        offset += (imports[i].imports_count + 1) * 8;
    }

    offset += func_index * 8;

    return iat_base_rva + offset;
}

THRIVE_API THRIVE_INLINE u32 thrive_pe32_plus_calculate_import_function_count(
    thrive_p32_plus_import *imports,
    u32 num_imports)
{
    u32 result = 0;

    u32 i;

    for (i = 0; i < num_imports; ++i)
    {
        result += imports[i].imports_count;
    }

    return result;
}

THRIVE_API THRIVE_INLINE u32 thrive_pe32_plus_calculate_import_strings_size(
    thrive_p32_plus_import *imports,
    u32 num_imports)
{
    u32 result = 0;

    u32 i;
    u32 j;

    for (i = 0; i < num_imports; ++i)
    {
        result += thrive_string_length(imports[i].library) + 1;

        for (j = 0; j < imports[i].imports_count; ++j)
        {
            result += 2 + thrive_string_length((s8 *)imports[i].imports[j]) + 1; /* 2 bytes for Hint */
        }
    }

    return result;
}

void thrive_pe32_plus_generate(
    thrive_buffer *out,
    thrive_buffer *code,
    thrive_p32_plus_import *imports,
    u32 num_imports,
    u32 text_vsize)
{
    u32 i, j;
    u32 num_funcs = thrive_pe32_plus_calculate_import_function_count(imports, num_imports);
    u32 strings_size = thrive_pe32_plus_calculate_import_strings_size(imports, num_imports);

    /* Section Layout Calculations */
    u32 idt_size = (num_imports + 1) * 20;
    u32 ilt_size = (num_funcs + num_imports) * 8;
    u32 iat_size = ilt_size;

    u32 text_rva = 0x1000;
    u32 text_raw_size = thrive_align_up(code->size, 0x200);

    u32 rdata_rva = text_rva + thrive_align_up(text_vsize, 0x1000);
    u32 rdata_vsize = idt_size + ilt_size + iat_size + strings_size;
    u32 rdata_raw_size = thrive_align_up(rdata_vsize, 0x200);

    u32 idt_rva = rdata_rva;
    u32 ilt_rva = idt_rva + idt_size;
    u32 iat_rva = ilt_rva + ilt_size;
    u32 strings_rva = iat_rva + iat_size;
    u32 size_of_image = thrive_align_up(rdata_rva + rdata_vsize, 0x1000);

    u32 current_dll_name_rva = strings_rva;
    u32 current_func_name_rva = strings_rva;
    u32 current_ilt_rva = ilt_rva;
    u32 current_iat_rva = iat_rva;

    /* Write DOS Header */
    thrive_buffer_write_u16(out, 0x5A4D); /* MZ */
    thrive_buffer_align(out, 0x3C);       /* */
    thrive_buffer_write_u32(out, 0x40);   /* e_lfanew */

    /* Write NT Headers */
    thrive_buffer_write_u32(out, 0x00004550); /* PE\0\0 */
    thrive_buffer_write_u16(out, 0x8664);     /* Machine: AMD64 */
    thrive_buffer_write_u16(out, 2);          /* NumberOfSections */
    thrive_buffer_write_u32(out, 0);          /* TimeDateStamp */
    thrive_buffer_write_u32(out, 0);          /* PointerToSymbolTable */
    thrive_buffer_write_u32(out, 0);          /* NumberOfSymbols */
    thrive_buffer_write_u16(out, 0xF0);       /* SizeOfOptionalHeader */
    thrive_buffer_write_u16(out, 0x0022);     /* Characteristics (Exec, LargeAddr) */

    /* Write Optional Header (PE32+) */
    thrive_buffer_write_u16(out, 0x020B);         /* Magic */
    thrive_buffer_write_u8(out, 1);               /* MajorLinkerVersion */
    thrive_buffer_write_u8(out, 0);               /* MinorLinkerVersion */
    thrive_buffer_write_u32(out, text_raw_size);  /* SizeOfCode */
    thrive_buffer_write_u32(out, rdata_raw_size); /* SizeOfInitializedData */
    thrive_buffer_write_u32(out, 0);              /* SizeOfUninitializedData */
    thrive_buffer_write_u32(out, text_rva);       /* AddressOfEntryPoint */
    thrive_buffer_write_u32(out, text_rva);       /* BaseOfCode */

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlong-long"
    thrive_buffer_write_u64(out, 0x0000000140000000ULL); /* ImageBase */
#pragma GCC diagnostic pop

    thrive_buffer_write_u32(out, 0x1000);        /* SectionAlignment */
    thrive_buffer_write_u32(out, 0x200);         /* FileAlignment */
    thrive_buffer_write_u16(out, 5);             /* */
    thrive_buffer_write_u16(out, 2);             /* OSMajor / OSMinor */
    thrive_buffer_write_u16(out, 0);             /* */
    thrive_buffer_write_u16(out, 0);             /* ImageMajor / ImageMinor */
    thrive_buffer_write_u16(out, 5);             /* */
    thrive_buffer_write_u16(out, 2);             /* SubSysMajor / SubSysMinor */
    thrive_buffer_write_u32(out, 0);             /* Win32VersionValue */
    thrive_buffer_write_u32(out, size_of_image); /* SizeOfImage */
    thrive_buffer_write_u32(out, 0x200);         /* SizeOfHeaders */
    thrive_buffer_write_u32(out, 0);             /* CheckSum */
    thrive_buffer_write_u16(out, 3);             /* Subsystem (3 = Windows CUI Console, 2 = Windows GUI, 1 = Native) */
    thrive_buffer_write_u16(out, 0x8140);        /* DllCharacteristics (NX, DynBase, HighEnt) */
    thrive_buffer_write_u64(out, 0x100000);      /* SizeOfStackReserve */
    thrive_buffer_write_u64(out, 0x1000);        /* SizeOfStackCommit */
    thrive_buffer_write_u64(out, 0x100000);      /* SizeOfHeapReserve */
    thrive_buffer_write_u64(out, 0x1000);        /* SizeOfHeapCommit */
    thrive_buffer_write_u32(out, 0);             /* LoaderFlags */
    thrive_buffer_write_u32(out, 16);            /* NumberOfRvaAndSizes */

    /* Data Directories */
    thrive_buffer_write_u32(out, 0);
    thrive_buffer_write_u32(out, 0); /* Export Directory */
    thrive_buffer_write_u32(out, idt_rva);
    thrive_buffer_write_u32(out, idt_size); /* Import Directory */

    for (i = 2; i < 16; ++i)
    {
        thrive_buffer_write_u32(out, 0);
        thrive_buffer_write_u32(out, 0);
    }

    /* Section Header: .text */
    thrive_buffer_write_bytes(out, (u8 *)".text\0\0\0", 8);
    thrive_buffer_write_u32(out, code->size);    /* VirtualSize */
    thrive_buffer_write_u32(out, text_rva);      /* VirtualAddress */
    thrive_buffer_write_u32(out, text_raw_size); /* SizeOfRawData */
    thrive_buffer_write_u32(out, 0x200);         /* PointerToRawData */
    thrive_buffer_write_u32(out, 0);             /* */
    thrive_buffer_write_u32(out, 0);             /* */
    thrive_buffer_write_u16(out, 0);             /* */
    thrive_buffer_write_u16(out, 0);             /* */
    thrive_buffer_write_u32(out, 0x60000020);    /* Characteristics (RX) */

    /* Section Header: .rdata */
    thrive_buffer_write_bytes(out, (u8 *)".rdata\0\0", 8);
    thrive_buffer_write_u32(out, rdata_vsize);           /* VirtualSize */
    thrive_buffer_write_u32(out, rdata_rva);             /* VirtualAddress */
    thrive_buffer_write_u32(out, rdata_raw_size);        /* SizeOfRawData */
    thrive_buffer_write_u32(out, 0x200 + text_raw_size); /* PointerToRawData */
    thrive_buffer_write_u32(out, 0);                     /* */
    thrive_buffer_write_u32(out, 0);                     /* */
    thrive_buffer_write_u16(out, 0);                     /* */
    thrive_buffer_write_u16(out, 0);                     /* */
    thrive_buffer_write_u32(out, 0x40000040);            /* Characteristics (R) */

    thrive_buffer_align(out, 0x200); /* Pad Header to File Alignment */

    /* Write .text Data */
    thrive_buffer_write_bytes(out, code->data, code->size);
    thrive_buffer_align(out, 0x200);

    /* Write .rdata Data (Imports) */
    for (i = 0; i < num_imports; ++i)
    {
        current_func_name_rva += thrive_string_length(imports[i].library) + 1;
    }

    /* Write Import Descriptor Table (IDT) */
    for (i = 0; i < num_imports; ++i)
    {
        thrive_buffer_write_u32(out, current_ilt_rva);
        thrive_buffer_write_u32(out, 0);
        thrive_buffer_write_u32(out, 0);
        thrive_buffer_write_u32(out, current_dll_name_rva);
        thrive_buffer_write_u32(out, current_iat_rva);

        current_dll_name_rva += thrive_string_length(imports[i].library) + 1;
        current_ilt_rva += (imports[i].imports_count + 1) * 8;
        current_iat_rva += (imports[i].imports_count + 1) * 8;
    }

    thrive_buffer_write_u32(out, 0);
    thrive_buffer_write_u32(out, 0);
    thrive_buffer_write_u32(out, 0);
    thrive_buffer_write_u32(out, 0);
    thrive_buffer_write_u32(out, 0);

    /* Write Import Lookup Table (ILT) & Import Address Table (IAT) */
    {
        i32 table;

        for (table = 0; table < 2; ++table)
        {
            u32 temp_name_rva = current_func_name_rva;

            for (i = 0; i < num_imports; ++i)
            {
                for (j = 0; j < imports[i].imports_count; ++j)
                {
                    thrive_buffer_write_u64(out, temp_name_rva);
                    temp_name_rva += 2 + thrive_string_length((s8 *)imports[i].imports[j]) + 1;
                }
                thrive_buffer_write_u64(out, 0); /* Null thunk bounds the array */
            }
        }

        /* Write Import Strings: DLL Names */
        for (i = 0; i < num_imports; ++i)
        {
            thrive_buffer_write_bytes(out, (u8 *)imports[i].library, thrive_string_length(imports[i].library) + 1);
        }

        /* Write Import Strings: Function Names */
        for (i = 0; i < num_imports; ++i)
        {
            for (j = 0; j < imports[i].imports_count; ++j)
            {
                thrive_buffer_write_u16(out, 0); /* Ordinal Hint */
                thrive_buffer_write_bytes(out, (u8 *)imports[i].imports[j], thrive_string_length((s8 *)imports[i].imports[j]) + 1);
            }
        }
    }

    /* Final File Alignment Pad */
    thrive_buffer_align(out, 0x200);
}

/* #############################################################################
 * # [SECTION] X86_64 Machine Code Emitter
 * #############################################################################
 */
THRIVE_API THRIVE_INLINE void thrive_x64_modrm_reg(thrive_buffer *b, thrive_x64_reg reg, thrive_x64_reg rm)
{
    /* Mod = 11 (register-direct addressing) */
    u8 modrm = 0xC0 | ((reg & 7) << 3) | (rm & 7);
    thrive_buffer_write_u8(b, modrm);
}

THRIVE_API THRIVE_INLINE void thrive_x64_rex(thrive_buffer *b, u8 w, thrive_x64_reg reg, thrive_x64_reg rm)
{
    u8 rex = 0x40;

    if (w)
    {
        rex |= 0x08; /* W: 64-bit operand size */
    }

    if (reg >= 8)
    {
        rex |= 0x04; /* R: Extension for 'reg' field */
    }

    if (rm >= 8)
    {
        rex |= 0x01; /* B: Extension for 'rm' field */
    }

    thrive_buffer_write_u8(b, rex);
}

/* SUB reg, imm8 or ADD reg, imm8 */
THRIVE_API THRIVE_INLINE void thrive_x64_alu_ri8(thrive_buffer *b, thrive_x64_op_ext op_ext, thrive_x64_reg dst, u8 imm)
{
    thrive_x64_rex(b, 1, 0, dst);
    thrive_buffer_write_u8(b, 0x83);
    thrive_buffer_write_u8(b, (u8)(0xC0 | (op_ext << 3) | (dst & 7)));
    thrive_buffer_write_u8(b, imm);
}

/* [rbp + disp32] */
THRIVE_API THRIVE_INLINE void thrive_x64_modrm_disp32(thrive_buffer *b, thrive_x64_reg reg, thrive_x64_reg base, i32 disp)
{
    thrive_buffer_write_u8(b, 0x80 | ((reg & 7) << 3) | (base & 7)); /* mod=10 */
    thrive_buffer_write_u32(b, (u32)disp);
}

/* MOV reg, imm32 */
THRIVE_API THRIVE_INLINE void thrive_x64_mov_ri32(thrive_buffer *b, thrive_x64_reg dst, u32 imm)
{
    thrive_x64_rex(b, 1, 0, dst);
    thrive_buffer_write_u8(b, 0xC7);
    thrive_buffer_write_u8(b, 0xC0 | (OP_EXT_MOV << 3) | (dst & 7));
    thrive_buffer_write_u32(b, imm);
}

/* MOV reg, imm64 */
THRIVE_API THRIVE_INLINE void thrive_x64_mov_ri64(thrive_buffer *b, thrive_x64_reg dst, u64 imm)
{
    thrive_x64_rex(b, 1, 0, dst);
    thrive_buffer_write_u8(b, 0xB8 | (dst & 7));
    thrive_buffer_write_u64(b, imm);
}

/* mov r64, [rbp+disp] */
THRIVE_API THRIVE_INLINE void thrive_x64_mov_r_mrbp(thrive_buffer *b, thrive_x64_reg dst, i32 disp)
{
    thrive_x64_rex(b, 1, dst, REG_RBP);
    thrive_buffer_write_u8(b, 0x8B);
    thrive_x64_modrm_disp32(b, dst, REG_RBP, disp);
}

/* mov [rbp+disp], r64 */
THRIVE_API THRIVE_INLINE void thrive_x64_mov_mrbp_r(thrive_buffer *b, i32 disp, thrive_x64_reg src)
{
    thrive_x64_rex(b, 1, src, REG_RBP);
    thrive_buffer_write_u8(b, 0x89);
    thrive_x64_modrm_disp32(b, src, REG_RBP, disp);
}

/* lea r64, [rbp+disp] */
THRIVE_API THRIVE_INLINE void thrive_x64_lea_r_mrbp(thrive_buffer *b, thrive_x64_reg dst, i32 disp)
{
    thrive_x64_rex(b, 1, dst, REG_RBP);
    thrive_buffer_write_u8(b, 0x8D);
    thrive_x64_modrm_disp32(b, dst, REG_RBP, disp);
}

/* MOV reg, reg */
THRIVE_API THRIVE_INLINE void thrive_x64_mov_rr(thrive_buffer *b, thrive_x64_reg dst, thrive_x64_reg src)
{
    thrive_x64_rex(b, 1, dst, src);
    thrive_buffer_write_u8(b, 0x8B);
    thrive_x64_modrm_reg(b, dst, src);
}

/* mov rax, [rax] */
THRIVE_API THRIVE_INLINE void thrive_x64_mov_r_mr(thrive_buffer *b, thrive_x64_reg dst, thrive_x64_reg base)
{
    thrive_x64_rex(b, 1, dst, base);
    thrive_buffer_write_u8(b, 0x8B);
    thrive_x64_modrm_reg(b, dst, base);
}

/* mov [rax], rbx */
THRIVE_API THRIVE_INLINE void thrive_x64_mov_mr_r(thrive_buffer *b, thrive_x64_reg base, thrive_x64_reg src)
{
    thrive_x64_rex(b, 1, src, base);
    thrive_buffer_write_u8(b, 0x89);
    thrive_x64_modrm_reg(b, src, base);
}

/* movzx rax, al */
THRIVE_API THRIVE_INLINE void thrive_x64_movzx_rax_al(thrive_buffer *b)
{
    thrive_buffer_write_u8(b, 0x48);
    thrive_buffer_write_u8(b, 0x0F);
    thrive_buffer_write_u8(b, 0xB6);
    thrive_buffer_write_u8(b, 0xC0);
}

/* PUSH reg */
THRIVE_API THRIVE_INLINE void thrive_x64_push_r(thrive_buffer *b, thrive_x64_reg reg)
{
    if (reg >= 8)
    {
        thrive_buffer_write_u8(b, 0x41); /* REX.B for high registers */
    }
    thrive_buffer_write_u8(b, 0x50 | (reg & 7));
}

/* POP reg */
THRIVE_API THRIVE_INLINE void thrive_x64_pop_r(thrive_buffer *b, thrive_x64_reg reg)
{
    if (reg >= 8)
    {
        thrive_buffer_write_u8(b, 0x41);
    }
    thrive_buffer_write_u8(b, 0x58 | (reg & 7));
}

/* XOR reg, reg */
THRIVE_API THRIVE_INLINE void thrive_x64_xor_rr(thrive_buffer *b, thrive_x64_reg dst, thrive_x64_reg src)
{
    thrive_x64_rex(b, 1, src, dst);
    thrive_buffer_write_u8(b, 0x31);
    thrive_x64_modrm_reg(b, src, dst);
}

/* add r64, r64 */
THRIVE_API THRIVE_INLINE void thrive_x64_add_rr(thrive_buffer *b, thrive_x64_reg dst, thrive_x64_reg src)
{
    thrive_x64_rex(b, 1, src, dst);
    thrive_buffer_write_u8(b, 0x01);
    thrive_x64_modrm_reg(b, src, dst);
}

/* sub r64, r64 */
THRIVE_API THRIVE_INLINE void thrive_x64_sub_rr(thrive_buffer *b, thrive_x64_reg dst, thrive_x64_reg src)
{
    thrive_x64_rex(b, 1, src, dst);
    thrive_buffer_write_u8(b, 0x29);
    thrive_x64_modrm_reg(b, src, dst);
}

/* imul r64, r64 */
THRIVE_API THRIVE_INLINE void thrive_x64_imul_rr(thrive_buffer *b, thrive_x64_reg dst, thrive_x64_reg src)
{
    thrive_x64_rex(b, 1, dst, src);
    thrive_buffer_write_u8(b, 0x0F);
    thrive_buffer_write_u8(b, 0xAF);
    thrive_x64_modrm_reg(b, dst, src);
}

/* cmp r64, r64 */
THRIVE_API THRIVE_INLINE void thrive_x64_cmp_rr(thrive_buffer *b, thrive_x64_reg a, thrive_x64_reg rb)
{
    thrive_x64_rex(b, 1, rb, a);
    thrive_buffer_write_u8(b, 0x39);
    thrive_x64_modrm_reg(b, rb, a);
}

/* setcc */
THRIVE_API THRIVE_INLINE void thrive_x64_setcc(thrive_buffer *b, thrive_x64_cc cc)
{
    thrive_buffer_write_u8(b, 0x0F);
    thrive_buffer_write_u8(b, (u8)(0x90 | cc)); /* cc = condition */
    thrive_buffer_write_u8(b, 0xC0);            /* AL */
}

/* jmp rel32 */
THRIVE_API THRIVE_INLINE void thrive_x64_jmp(thrive_buffer *b, i32 rel)
{
    thrive_buffer_write_u8(b, 0xE9);
    thrive_buffer_write_u32(b, (u32)rel);
}

/* je / jne / etc */
THRIVE_API THRIVE_INLINE void thrive_x64_jcc(thrive_buffer *b, thrive_x64_cc cc, i32 rel)
{
    thrive_buffer_write_u8(b, 0x0F);
    thrive_buffer_write_u8(b, (u8)(0x80 | cc));
    thrive_buffer_write_u32(b, (u32)rel);
}

/* NEG */
THRIVE_API THRIVE_INLINE void thrive_x64_neg_r(thrive_buffer *b, thrive_x64_reg reg)
{
    thrive_x64_rex(b, 1, 0, reg);
    thrive_buffer_write_u8(b, 0xF7);
    thrive_buffer_write_u8(b, 0xD8 | (reg & 7));
}

/* TEST rax, rax */
THRIVE_API THRIVE_INLINE void thrive_x64_test_rr(thrive_buffer *b, thrive_x64_reg a, thrive_x64_reg breg)
{
    thrive_x64_rex(b, 1, breg, a);
    thrive_buffer_write_u8(b, 0x85);
    thrive_x64_modrm_reg(b, breg, a);
}

/* CMP rax, imm32 */
THRIVE_API THRIVE_INLINE void thrive_x64_cmp_ri32(thrive_buffer *b, thrive_x64_reg reg, u32 imm)
{
    thrive_x64_rex(b, 1, 0, reg);
    thrive_buffer_write_u8(b, 0x81);
    thrive_buffer_write_u8(b, 0xF8 | (reg & 7));
    thrive_buffer_write_u32(b, imm);
}

THRIVE_API THRIVE_INLINE void thrive_x64_and_rr(thrive_buffer *b, thrive_x64_reg dst, thrive_x64_reg src)
{
    thrive_x64_rex(b, 1, src, dst);
    thrive_buffer_write_u8(b, 0x21);
    thrive_x64_modrm_reg(b, src, dst);
}

THRIVE_API THRIVE_INLINE void thrive_x64_or_rr(thrive_buffer *b, thrive_x64_reg dst, thrive_x64_reg src)
{
    thrive_x64_rex(b, 1, src, dst);
    thrive_buffer_write_u8(b, 0x09);
    thrive_x64_modrm_reg(b, src, dst);
}

THRIVE_API THRIVE_INLINE void thrive_x64_shl_cl(thrive_buffer *b, thrive_x64_reg reg)
{
    thrive_x64_rex(b, 1, 0, reg);
    thrive_buffer_write_u8(b, 0xD3);
    thrive_buffer_write_u8(b, 0xE0 | (reg & 7));
}

THRIVE_API THRIVE_INLINE void thrive_x64_shr_cl(thrive_buffer *b, thrive_x64_reg reg)
{
    thrive_x64_rex(b, 1, 0, reg);
    thrive_buffer_write_u8(b, 0xD3);
    thrive_buffer_write_u8(b, 0xE8 | (reg & 7));
}

/* IDIV */
THRIVE_API THRIVE_INLINE void thrive_x64_idiv_r(thrive_buffer *b, thrive_x64_reg reg)
{
    thrive_x64_rex(b, 1, 0, reg);
    thrive_buffer_write_u8(b, 0xF7);
    thrive_buffer_write_u8(b, 0xF8 | (reg & 7));
}

/* CQO */
THRIVE_API THRIVE_INLINE void thrive_x64_cqo(thrive_buffer *b)
{
    thrive_buffer_write_u8(b, 0x48);
    thrive_buffer_write_u8(b, 0x99);
}

THRIVE_API THRIVE_INLINE void thrive_x64_leave(thrive_buffer *b)
{
    thrive_buffer_write_u8(b, 0xC9);
}

THRIVE_API THRIVE_INLINE void thrive_x64_ret(thrive_buffer *b)
{
    thrive_buffer_write_u8(b, 0xC3);
}

/* lea rax, [rel STR_0] */
THRIVE_API THRIVE_INLINE void thrive_x64_lea_rip_rel32(thrive_buffer *b, thrive_x64_reg dst, u32 rel)
{
    thrive_x64_rex(b, 1, dst, 0);
    thrive_buffer_write_u8(b, 0x8D);
    thrive_buffer_write_u8(b, 0x05 | ((dst & 7) << 3));
    thrive_buffer_write_u32(b, rel);
}

/* direct call */
THRIVE_API THRIVE_INLINE void thrive_x64_call_rel32(thrive_buffer *b, u32 curr_rva, u32 target_rva)
{
    u32 rel = target_rva - (curr_rva + 5);
    thrive_buffer_write_u8(b, 0xE8);
    thrive_buffer_write_u32(b, rel);
}

THRIVE_API THRIVE_INLINE void thrive_x64_inst_call_rel32(thrive_buffer *b, u32 curr_rva, u32 target_rva)
{
    u32 rel = target_rva - (curr_rva + 6);
    thrive_buffer_write_u8(b, 0xFF); /* CALL */
    thrive_buffer_write_u8(b, 0x15); /* ModR/M: rel32 */
    thrive_buffer_write_u32(b, rel);
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
