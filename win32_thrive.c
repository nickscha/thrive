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

/* Memory Management */
#define MEM_COMMIT 0x00001000
#define MEM_RESERVE 0x00002000
#define MEM_RELEASE 0x00008000
#define PAGE_READWRITE 0x04

/* Console IO */
#define STD_OUTPUT_HANDLE ((unsigned long)-11)

/* File IO */
#define INVALID_HANDLE ((void *)-1)
#define GENERIC_READ (0x80000000L)
#define FILE_SHARE_READ 0x00000001
#define OPEN_EXISTING 3
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_FLAG_SEQUENTIAL_SCAN 0x08000000
#define INVALID_FILE_SIZE ((unsigned long)0xFFFFFFFF)

/* File Memory Mapping */
#define PAGE_READONLY 0x02
#define FILE_MAP_READ 0x0004

/* Memory Management */
WIN32_API(void *)
VirtualAlloc(void *lpAddress, unsigned int dwSize, unsigned long flAllocationType, unsigned long flProtect);
WIN32_API(int)
VirtualFree(void *lpAddress, unsigned int dwSize, unsigned long dwFreeType);

/* Console IO */
WIN32_API(void *)
GetStdHandle(unsigned long nStdHandle);
WIN32_API(int)
WriteConsoleA(void *hConsoleOutput, void *lpBuffer, unsigned long nNumberOfCharsToWrite, unsigned long *lpNumberOfCharsWritten, void *lpReserved);
WIN32_API(char *)
GetCommandLineA(void);

/* File IO */
WIN32_API(int)
CloseHandle(void *hObject);
WIN32_API(void *)
CreateFileA(char *lpFileName, unsigned long dwDesiredAccess, unsigned long dwShareMode, void *, unsigned long dwCreationDisposition, unsigned long dwFlagsAndAttributes, void *hTemplateFile);
WIN32_API(int)
ReadFile(void *hFile, void *lpBuffer, unsigned long nNumberOfBytesToRead, unsigned long *lpNumberOfBytesRead, void *lpOverlapped);
WIN32_API(unsigned long)
GetFileSize(void *hFile, unsigned long *lpFileSizeHigh);

/* File Memory Mapping */
WIN32_API(void *)
CreateFileMappingA(void *hFile, void *lpFileMappingAttributes, unsigned long flProtect, unsigned long dwMaximumSizeHigh, unsigned long dwMaximumSizeLow, char *lpName);
WIN32_API(void *)
MapViewOfFile(void *hFileMappingObject, unsigned long dwDesiredAccess, unsigned long dwFileOffsetHigh, unsigned long dwFileOffsetLow, unsigned long dwNumberOfBytesToMap);

/* Performance Metrics */
typedef struct LARGE_INTEGER
{
    unsigned long LowPart;
    long HighPart;

} LARGE_INTEGER;

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

#define FILE_MMAP_THRESHOLD (1024 * 1024) /* 1 MB */

THRIVE_API u8 *win32_io_file_read(
    char *filename,
    u32 *file_size_out)
{
    void *hFile;
    unsigned long fileSize;
    unsigned long bytesRead;
    u8 *buffer = 0;

    hFile = CreateFileA(
        filename,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        0);

    if (hFile == INVALID_HANDLE)
    {
        return 0;
    }

    fileSize = GetFileSize(hFile, 0);

    if (fileSize == INVALID_FILE_SIZE)
    {
        CloseHandle(hFile);
        return 0;
    }

    if (fileSize <= FILE_MMAP_THRESHOLD)
    {
        /* Small file: normal ReadFile */
        buffer = (u8 *)VirtualAlloc(0, fileSize + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!buffer)
        {
            CloseHandle(hFile);
            return 0;
        }

        if (!ReadFile(hFile, buffer, fileSize, &bytesRead, 0) || bytesRead != fileSize)
        {
            CloseHandle(hFile);
            VirtualFree(buffer, 0, MEM_RELEASE);
            return 0;
        }

        buffer[fileSize] = '\0';
    }
    else
    {
        /* Large file: memory-mapped file */
        void *hMap = CreateFileMappingA(hFile, 0, PAGE_READONLY, 0, 0, 0);

        if (!hMap)
        {
            CloseHandle(hFile);
            return 0;
        }

        buffer = (u8 *)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);

        CloseHandle(hMap);

        if (!buffer)
        {
            CloseHandle(hFile);
            return 0;
        }
    }

    *file_size_out = fileSize;
    CloseHandle(hFile);
    return buffer;
}

THRIVE_API void win32_io_print_ms(void *hConsole, u8 *name, u32 name_length, f64 ms, f64 ms_total)
{
    unsigned long written;

    u8 buf[128];
    u8 tmp[16];
    u8 *p = buf;
    u32 i;
    f64 ms_mid = 0.02;
    f64 ms_high = 0.4;

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
    *p++ = 'm';
    *p++ = 's';

    /* print percentage with 1 decimal place (padded right) */
    {
        f64 percent = (ms_total > 0.0) ? (ms / ms_total * 100.0) : 0.0;
        i32 whole = (i32)percent;
        i32 frac = (i32)((percent - (f64)whole) * 10.0); /* one decimal */

        if (frac < 0)
        {
            frac = 0;
        }
        else if (frac > 9)
        {
            frac = 9;
        }

        *p++ = ' ';

        if (whole < 10)
        {
            *p++ = ' ';
            *p++ = ' ';
        }
        else if (whole < 100)
        {
            *p++ = ' ';
        }

        /* integer part */
        {
            i32 n = 0;
            do
            {
                tmp[n++] = (u8)('0' + (whole % 10));
                whole /= 10;
            } while (whole > 0);

            while (n--)
            {
                *p++ = tmp[n];
            }
        }

        *p++ = '.';
        *p++ = (u8)('0' + frac);
        *p++ = '%';
    }

    *p++ = '\n';

    {
        u32 total_len = (u32)(p - buf);
        u32 remain = total_len - 8;
        u32 cut = (remain > name_length + 2 ? name_length + 2 : remain);
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
 * # Performance Metrics
 * ############################################################################
 */
typedef enum win32_thrive_metrics
{
    METRIC_IO_FILE_READ = 0,
    METRIC_TOKENIZATION,
    METRIC_AST,
    METRIC_ASM,
    METRIC_AST_OPTIMIZED,
    METRIC_ASM_OPTIMIZED,
    METRIC_COUNT

} win32_thrive_metrics;

typedef struct win32_thrive_metric
{
    LARGE_INTEGER time_start;
    LARGE_INTEGER time_end;

} win32_thrive_metric;

/* ############################################################################
 * # Thrive Compilation
 * ############################################################################
 */
THRIVE_API void thrive_compile(win32_thrive_metric *metrics)
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

    (void)thrive_token_type_names;

    /* Generate Tokens */
    QueryPerformanceCounter(&metrics[METRIC_TOKENIZATION].time_start);
    thrive_tokenizer(code, code_size, tokens, TOKENS_CAPACITY, &tokens_size);
    QueryPerformanceCounter(&metrics[METRIC_TOKENIZATION].time_end);

    /* Generate AST */
    QueryPerformanceCounter(&metrics[METRIC_AST].time_start);
    thrive_ast_parse(tokens, tokens_size, ast, AST_CAPACITY, &ast_size);
    QueryPerformanceCounter(&metrics[METRIC_AST].time_end);

    /* Generate Assembly */
    QueryPerformanceCounter(&metrics[METRIC_ASM].time_start);
    thrive_codegen(ast, ast_size, code_asm, CODE_CAPACITY, &code_asm_size);
    QueryPerformanceCounter(&metrics[METRIC_ASM].time_end);

    /* Optimize AST */
    QueryPerformanceCounter(&metrics[METRIC_AST_OPTIMIZED].time_start);
    thrive_ast_optimize(ast, &ast_size);
    QueryPerformanceCounter(&metrics[METRIC_AST_OPTIMIZED].time_end);

    /* Generate Optimized Assembly */
    QueryPerformanceCounter(&metrics[METRIC_ASM_OPTIMIZED].time_start);
    thrive_codegen(ast, ast_size, code_asm, CODE_CAPACITY, &code_asm_size);
    QueryPerformanceCounter(&metrics[METRIC_ASM_OPTIMIZED].time_end);
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
    unsigned long written = 0;
    void *hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    LARGE_INTEGER freq;

    win32_thrive_metric metrics[METRIC_COUNT];

    u32 file_size = 0;
    u8 *file_memory;
    char *file_name;

    /* Print usage */
    if (argc < 2)
    {
        WriteConsoleA(hConsole, "[thrive] usage: ", 16, &written, 0);
        WriteConsoleA(hConsole, argv[0], (unsigned long)win32_strlen(argv[0]), &written, 0);
        WriteConsoleA(hConsole, " code.thrive\n", 13, &written, 0);
        return 1;
    }

    /* Query the CPU Frequency once */
    QueryPerformanceFrequency(&freq);

    file_name = (char *)argv[1];

    /* Read entire file */
    QueryPerformanceCounter(&metrics[METRIC_IO_FILE_READ].time_start);
    file_memory = win32_io_file_read(file_name, &file_size);
    QueryPerformanceCounter(&metrics[METRIC_IO_FILE_READ].time_end);

    if (!file_memory)
    {
        SetConsoleTextAttribute(hConsole, 12); /* red */
        WriteConsoleA(hConsole, "[thrive] Cannot read file!", 26, &written, 0);
        SetConsoleTextAttribute(hConsole, 7);
        return 1;
    }

    /* Compile */
    thrive_compile(metrics);

    /* Gather metrics */
    {
        u32 i;
        f64 metric_times[METRIC_COUNT];
        f64 metric_times_total = 0.0;

        /* Has to match with enum structure */
        u8 *win32_thrive_metric_names[] = {
            (u8 *)"time_io_file_read ",
            (u8 *)"time_tokenization ",
            (u8 *)"time_ast          ",
            (u8 *)"time_asm          ",
            (u8 *)"time_ast_optimized",
            (u8 *)"time_asm_optimized"};

        /* Compute total */
        for (i = 0; i < METRIC_COUNT; ++i)
        {
            f64 elapsed_ms = win32_elapsed_ms(&metrics[i].time_start, &metrics[i].time_end, &freq);
            metric_times[i] = elapsed_ms;
            metric_times_total += elapsed_ms;
        }

        /* Metric time */
        for (i = 0; i < METRIC_COUNT; ++i)
        {
            u8 *metric_name = win32_thrive_metric_names[i];
            win32_io_print_ms(hConsole, metric_name, win32_strlen(metric_name), metric_times[i], metric_times_total);
        }

        /* Total time */
        win32_io_print_ms(hConsole, (u8 *)"time_total        ", 18, metric_times_total, metric_times_total);
    }

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
