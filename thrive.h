/* thrive.h - v0.1 - public domain data structures - nickscha 2025

A C89 standard compliant, nostdlib (no C Standard Library) Low Level Programming Language inbetween Assembly and C (THRIVE).

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#ifndef THRIVE_H
#define THRIVE_H

/* #############################################################################
 * # COMPILER SETTINGS
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
