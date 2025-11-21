/* thrive.h - v0.2 - public domain data structures - nickscha 2025

A C89 standard compliant, nostdlib (no C Standard Library) Low Level Programming Language inbetween Assembly and C (THRIVE).

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#ifdef _MSC_VER
int _fltused = 0;
#endif

#ifdef _MSC_VER
#pragma function(memset)
#endif
void *memset(void *dest, int c, unsigned int count)
{
    char *bytes = (char *)dest;
    while (count--)
    {
        *bytes++ = (char)c;
    }
    return dest;
}

#ifdef _MSC_VER
#pragma function(memcpy)
#endif
void *memcpy(void *dest, void *src, unsigned int count)
{
    char *dest8 = (char *)dest;
    char *src8 = (char *)src;
    while (count--)
    {
        *dest8++ = *src8++;
    }
    return dest;
}

/* ############################################################################
 * # Manually define windows.h function we actually need
 * ############################################################################
 *
 * "windows.h" is huge (even with #NO_XXX #LEAN_.. defines).
 * This reduces compilation speed (especially with GCC/Clang) a lot since they,
 * compared to MSVC, do not precompile the windows header.
 *
 * Since most programs only use a small amount of win32 functions we can just
 * manually define them. This also lets us replace the DWORD, PTR, ... types
 * with standard C types.
 */
#ifndef _WINDOWS_
#define WIN32_API(r) __declspec(dllimport) r __stdcall

#define STD_OUTPUT_HANDLE ((unsigned long)-11)

typedef struct LARGE_INTEGER
{
    unsigned long LowPart;
    long HighPart;

} LARGE_INTEGER;

WIN32_API(void *)
GetStdHandle(unsigned long nStdHandle);
WIN32_API(int)
WriteConsoleA(void *hConsoleOutput, void *lpBuffer, unsigned long nNumberOfCharsToWrite, unsigned long *lpNumberOfCharsWritten, void *lpReserved);
WIN32_API(char *)
GetCommandLineA(void);
WIN32_API(int)
QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount);
WIN32_API(int)
QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency);
WIN32_API(int)
SetConsoleTextAttribute(void *hConsoleOutput, unsigned short wAttributes);
#endif /* _WINDOWS_ */

#include "thrive.h"

#define TOKENS_CAPACITY 1024
#define AST_CAPACITY 1024
#define CODE_CAPACITY 8192

THRIVE_API u32 win32_strlen(u8 *str)
{
    u8 *s = str;

    while (*s)
    {
        s++;
    }

    return (u32)(s - str);
}

THRIVE_API f64 win32_elapsed_ms(
    LARGE_INTEGER *start,
    LARGE_INTEGER *end,
    LARGE_INTEGER *freq)
{
    f64 delta = ((f64)end->HighPart - (f64)start->HighPart) * 4294967296.0 +
                ((f64)end->LowPart - (f64)start->LowPart);

    return (delta * 1000.0) / (f64)freq->LowPart;
}

THRIVE_API void win32_print_ms(void *hConsole, u8 *name, u32 name_length, f64 ms)
{
    unsigned long written;

    u8 buf[128];
    u8 tmp[16];
    u8 *p = buf;
    u32 i;
    f64 ms_mid = 0.02;
    f64 ms_high = 0.1;

    /* header: "[thrive] " */
    *p++ = '[';
    *p++ = 't';
    *p++ = 'h';
    *p++ = 'r';
    *p++ = 'i';
    *p++ = 'v';
    *p++ = 'e';
    *p++ = ']';
    *p++ = ' ';

    for (i = 0; i < name_length; ++i)
    {
        *p++ = name[i];
    }

    *p++ = ':';
    *p++ = ' ';

    /* print ms with 6 decimals */
    {
        i32 whole = (i32)ms;
        i32 frac = (i32)((ms - (f64)whole) * 1000000.0); /* 6 decimals */
        i32 n = 0;

        if (frac < 0)
        {
            frac = 0;
        }
        else if (frac > 999999)
        {
            frac = 999999;
        }

        /* convert integer part */
        do
        {
            tmp[n++] = (u8)('0' + (whole % 10));
            whole /= 10;
        } while (whole > 0);

        while (n--)
        {
            *p++ = tmp[n];
        }

        *p++ = '.';

        /* 6 padded decimals */
        *p++ = (u8)('0' + (frac / 100000) % 10);
        *p++ = (u8)('0' + (frac / 10000) % 10);
        *p++ = (u8)('0' + (frac / 1000) % 10);
        *p++ = (u8)('0' + (frac / 100) % 10);
        *p++ = (u8)('0' + (frac / 10) % 10);
        *p++ = (u8)('0' + (frac % 10));
    }

    /* suffix */
    *p++ = ' ';
    *p++ = 'm';
    *p++ = 's';
    *p++ = '\n';

    {
        u32 total_len = (u32)(p - buf);
        u32 remain = total_len - 8;
        u32 cut = (remain > 15 ? 15 : remain);
        u32 offset;

        /* color the "[thrive]" part only */
        SetConsoleTextAttribute(hConsole, 9);         /* blue */
        WriteConsoleA(hConsole, buf, 8, &written, 0); /* writes "[thrive]" */
        SetConsoleTextAttribute(hConsole, 7);         /* default */

        WriteConsoleA(hConsole, buf + 8, cut, &written, 0);

        offset = 8 + cut;

        SetConsoleTextAttribute(
            hConsole, ms >= ms_high  ? 12 /* red */
                      : ms >= ms_mid ? 14 /* yellow */
                                     : 10 /* green*/);

        WriteConsoleA(hConsole, buf + offset, total_len - offset, &written, 0);

        SetConsoleTextAttribute(hConsole, 7); /* default */
    }
}

/* ############################################################################
 * # Thrive Compilation
 * ############################################################################
 */
THRIVE_API void thrive_compile(void)
{

    u8 *code = (u8 *)"u32 a   = 42   \n"
                     "u32 b   = 27   \n"
                     "u32 res = a + b * 10.0f * (2 + 4)\n"
                     "ret res        \n";

    u32 code_size = thrive_strlen(code);

    thrive_token tokens[TOKENS_CAPACITY];
    u32 tokens_size = 0;

    thrive_ast ast[AST_CAPACITY];
    u32 ast_size = 0;

    u8 code_asm[CODE_CAPACITY];
    u32 code_asm_size = 0;

    void *hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    LARGE_INTEGER freq;
    LARGE_INTEGER time_start;
    LARGE_INTEGER time_end;
    f64 ms_current = 0.0;
    f64 ms_total = 0.0;

    (void)thrive_token_type_names;

    /* Query the CPU Frequency once */
    QueryPerformanceFrequency(&freq);

    /* Generate Tokens */
    QueryPerformanceCounter(&time_start);
    thrive_tokenizer(code, code_size, tokens, TOKENS_CAPACITY, &tokens_size);
    QueryPerformanceCounter(&time_end);
    ms_current = win32_elapsed_ms(&time_start, &time_end, &freq);
    ms_total += ms_current;
    win32_print_ms(hConsole, (u8 *)"time_token  ", 12, ms_current);

    /* Generate AST */
    QueryPerformanceCounter(&time_start);
    thrive_ast_parse(tokens, tokens_size, ast, AST_CAPACITY, &ast_size);
    QueryPerformanceCounter(&time_end);
    ms_current = win32_elapsed_ms(&time_start, &time_end, &freq);
    ms_total += ms_current;
    win32_print_ms(hConsole, (u8 *)"time_ast    ", 12, ms_current);

    /* Generate Assembly */
    QueryPerformanceCounter(&time_start);
    thrive_codegen(ast, ast_size, code_asm, CODE_CAPACITY, &code_asm_size);
    QueryPerformanceCounter(&time_end);
    ms_current = win32_elapsed_ms(&time_start, &time_end, &freq);
    ms_total += ms_current;
    win32_print_ms(hConsole, (u8 *)"time_asm    ", 12, ms_current);

    /* Optimize AST */
    QueryPerformanceCounter(&time_start);
    thrive_ast_optimize(ast, &ast_size);
    QueryPerformanceCounter(&time_end);
    ms_current = win32_elapsed_ms(&time_start, &time_end, &freq);
    ms_total += ms_current;
    win32_print_ms(hConsole, (u8 *)"time_ast_opt", 12, ms_current);

    /* Generate Optimized Assembly */
    QueryPerformanceCounter(&time_start);
    thrive_codegen(ast, ast_size, code_asm, CODE_CAPACITY, &code_asm_size);
    QueryPerformanceCounter(&time_end);
    ms_current = win32_elapsed_ms(&time_start, &time_end, &freq);
    ms_total += ms_current;
    win32_print_ms(hConsole, (u8 *)"time_asm_opt", 12, ms_current);

    win32_print_ms(hConsole, (u8 *)"time_total  ", 12, ms_total);
}

/* ############################################################################
 * # C-Like main function
 * ############################################################################
 *
 * The "mainCRTStartup" will read the command line arguments parsed and call
 * this function.
 */
THRIVE_API i32 start(i32 argc, u8 **argv)
{
    unsigned long written;
    void *hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    /* Print usage */
    if (argc < 2)
    {
        WriteConsoleA(hConsole, "[thrive] usage: ", 16, &written, 0);
        WriteConsoleA(hConsole, argv[0], (unsigned long)win32_strlen(argv[0]), &written, 0);
        WriteConsoleA(hConsole, " code.thrive\n", 13, &written, 0);
        return 1;
    }

    /* Compile */
    thrive_compile();

    return 0;
}

/* ############################################################################
 * # Command line parsing
 * ############################################################################
 *
 * Basic (non-quoted) command line parser
 * Converts "program.exe arg1 arg2" -> argc=3, argv={"program.exe","arg1","arg2",NULL}
 * In-place: modifies the command line buffer.
 */
THRIVE_API i32 win32_parse_command_line(u8 *cmdline, u8 ***argv_out)
{
    static u8 *argv_local[64]; /* up to 63 args */
    i32 argc = 0;

    while (*cmdline)
    {
        /* skip whitespace */
        while (*cmdline == ' ' || *cmdline == '\t')
        {
            cmdline++;
        }

        if (!*cmdline)
        {
            break;
        }

        if (argc < 63)
        {
            argv_local[argc++] = cmdline;
        }

        /* parse token (basic, no quote handling) */
        while (*cmdline && *cmdline != ' ' && *cmdline != '\t')
        {
            cmdline++;
        }

        if (*cmdline)
        {
            *cmdline++ = '\0';
        }
    }

    argv_local[argc] = (u8 *)0;
    *argv_out = argv_local;

    return argc;
}

/* ############################################################################
 * # Main entry point
 * ############################################################################
 */
#ifdef __clang__
#elif __GNUC__
__attribute((externally_visible))
#endif
#ifdef __i686__
__attribute((force_align_arg_pointer))
#endif
i32 nostdlib_main(void)
{
    u8 *cmdline = (u8 *)GetCommandLineA();
    u8 **argv;
    i32 argc = win32_parse_command_line(cmdline, &argv);

    return start(argc, argv);
}

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
