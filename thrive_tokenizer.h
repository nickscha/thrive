#ifndef THRIVE_TOKENIZER_H
#define THRIVE_TOKENIZER_H

#include "thrive.h"

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
        i32 integer;    /* valid if THRIVE_TOKEN_INTEGER */
        f64 floating;   /* valid if THRIVE_TOKEN_FLOAT   */
        u8 name[32];    /* valid if THRIVE_TOKEN_VAR     */
        u8 string[128]; /* valid if THRIVE_TOKEN_STRING  */

    } value;

} thrive_token;

THRIVE_API THRIVE_INLINE u8 thrive_is_identifier_char(u8 c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

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

        /* 2. Identifiers (Starts with Alpha or _) */
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')
        {
            thrive_token *tok = &tokens[*tokens_size];
            i32 len = 0;

            tok->cursor_pos = (u32)(cursor - code);
            tok->line_num = line_num;

            /* Fast copy loop */
            while (cursor < code_end && len < 31)
            {
                u8 nc = *cursor;

                if (!thrive_is_identifier_char(nc))
                {
                    break;
                }

                tok->value.name[len++] = nc;
                cursor++;
            }
            tok->value.name[len] = '\0';

            /* Fast Keyword Check (Length + Char comparison) */
            if (len == 3)
            {
                u8 *n = tok->value.name;

                if (n[0] == 'u' && n[1] == '3' && n[2] == '2')
                {
                    tok->type = THRIVE_TOKEN_KEYWORD_U32;
                }
                else if (n[0] == 'r' && n[1] == 'e' && n[2] == 't')
                {
                    tok->type = THRIVE_TOKEN_KEYWORD_RET;
                }
                else if (n[0] == 'e' && n[1] == 'x' && n[2] == 't')
                {
                    tok->type = THRIVE_TOKEN_KEYWORD_EXT;
                }
                else
                {
                    tok->type = THRIVE_TOKEN_VAR;
                }
            }
            else
            {
                tok->type = THRIVE_TOKEN_VAR;
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

        /* 4. String Literals */
        if (c == '"')
        {
            thrive_token *tok = &tokens[*tokens_size];
            i32 i = 0;

            tok->cursor_pos = (u32)(cursor - code);
            tok->line_num = line_num;

            cursor++; /* skip opening " */

            while (cursor < code_end && *cursor != '"' && i < 127)
            {
                u8 sc = *cursor;

                if (sc == '\\')
                {
                    cursor++;

                    if (cursor >= code_end)
                    {
                        break;
                    }

                    switch (*cursor)
                    {
                    case 'n':
                        tok->value.string[i++] = '\n';
                        break;
                    case 't':
                        tok->value.string[i++] = '\t';
                        break;
                    case 'r':
                        tok->value.string[i++] = '\r';
                        break;
                    case '"':
                        tok->value.string[i++] = '"';
                        break;
                    case '\\':
                        tok->value.string[i++] = '\\';
                        break;
                    default:
                        tok->value.string[i++] = *cursor;
                        break;
                    }
                }
                else
                {
                    tok->value.string[i++] = sc;
                }
                cursor++;
            }
            tok->value.string[i] = '\0';

            if (cursor < code_end && *cursor == '"')
            {
                cursor++;
            }

            tok->type = THRIVE_TOKEN_STRING;
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

#endif /* THRIVE_TOKENIZER_H */