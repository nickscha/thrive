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

THRIVE_API THRIVE_INLINE s8 thrive_char_is_whitespace(s8 c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
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

static s8 *stream;

THRIVE_API THRIVE_INLINE thrive_token thrive_token_next(void)
{
    thrive_token token = {0};

    token.start = stream;

    /* clang-format off */
    switch (*stream)
    {   
        /* Whitespaces */
        case ' ': case '\n': case '\r': case '\t':
        {
            while (thrive_char_is_whitespace(*stream)) {
                stream++;
            }

            return thrive_token_next();
        } 
        /* Number processing */
        case '0': case '1': case '2': case '3': case '4': case '5': case '6':
        case '7': case '8': case '9':
        {
            u32 value = 0;

            while (thrive_char_is_digit(*stream)) {
                value *= 10;
                value += (u32) (*stream++ - '0');
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
            while (thrive_char_is_alpha(*stream) ||
                   thrive_char_is_digit(*stream) ||
                   *stream == '_') 
            {
                stream++;
            }

            token.kind = THRIVE_TOKEN_KIND_NAME;

            {
                u32 token_length = (u32) (stream - token.start);

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
        case '(':  { stream++; token.kind = THRIVE_TOKEN_KIND_LPARAN; break; }
        case ')':  { stream++; token.kind = THRIVE_TOKEN_KIND_RPARAN; break; }
        case '=':  { stream++; token.kind = THRIVE_TOKEN_KIND_ASSIGN; break; }
        case '+':  { stream++; token.kind = THRIVE_TOKEN_KIND_ADD;    break; }
        case '-':  { stream++; token.kind = THRIVE_TOKEN_KIND_SUB;    break; }
        case '*':  { stream++; token.kind = THRIVE_TOKEN_KIND_MUL;    break; }
        case '/':  { stream++; token.kind = THRIVE_TOKEN_KIND_DIV;    break; }
        case '\0': { stream++; token.kind = THRIVE_TOKEN_KIND_EOF;    break; }
        default:   { stream++; token.kind = THRIVE_TOKEN_KIND_INVALID;break; }
    }
    /* clang-format on */

    token.end = stream;

    return token;
}

THRIVE_API THRIVE_INLINE thrive_status thrive_lexer(
    s8 *source_code,
    u32 source_code_size)
{
    thrive_status status = {0};

    if (!source_code)
    {
        status.type = THRIVE_STATUS_ERROR_ARGUMENTS;
        status.message = "No valid source code has been provided!\n";
        return status;
    }

    if (source_code_size < 1)
    {
        status.type = THRIVE_STATUS_ERROR_ARGUMENTS;
        status.message = "Source code is empty!\n";
        return status;
    }

    return status;
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
