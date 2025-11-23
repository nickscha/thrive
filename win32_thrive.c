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
 * manually define them. This also lets us replace the ancient DWORD, PTR, ... types
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
#define GENERIC_WRITE (0x40000000L)
#define CREATE_ALWAYS 2
#define FILE_SHARE_READ 0x00000001
#define FILE_SHARE_WRITE 0x00000002
#define FILE_SHARE_DELETE 0x00000004
#define OPEN_EXISTING 3
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_ATTRIBUTE_TEMPORARY 0x00000100
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
typedef struct FILETIME
{
    unsigned long dwLowDateTime;
    unsigned long dwHighDateTime;

} FILETIME;

typedef struct FILE_ATTRIBUTE_DATA
{
    unsigned long dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    unsigned long nFileSizeHigh;
    unsigned long nFileSizeLow;
} FILE_ATTRIBUTE_DATA;

typedef enum GET_FILEEX_INFO_LEVELS
{
    GetFileExInfoStandard,
    GetFileExMaxInfoLevel
} GET_FILEEX_INFO_LEVELS;

WIN32_API(int)
CloseHandle(void *hObject);
WIN32_API(int)
GetFileAttributesExA(char *lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, void *lpFileInformation);
WIN32_API(long)
CompareFileTime(FILETIME *lpFileTime1, FILETIME *lpFileTime2);
WIN32_API(void *)
CreateFileA(char *lpFileName, unsigned long dwDesiredAccess, unsigned long dwShareMode, void *, unsigned long dwCreationDisposition, unsigned long dwFlagsAndAttributes, void *hTemplateFile);
WIN32_API(unsigned long)
GetFileSize(void *hFile, unsigned long *lpFileSizeHigh);
WIN32_API(int)
ReadFile(void *hFile, void *lpBuffer, unsigned long nNumberOfBytesToRead, unsigned long *lpNumberOfBytesRead, void *lpOverlapped);
WIN32_API(int)
WriteFile(void *hFile, void *lpBuffer, unsigned long nNumberOfBytesToWrite, unsigned long *lpNumberOfBytesWritten, void *lpOverlapped);

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

WIN32_API(void)
Sleep(unsigned long dwMilliseconds);

#endif /* _WINDOWS_ */

#include "thrive.h"

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

THRIVE_API THRIVE_INLINE FILETIME win32_io_file_mod_time(char *file)
{
    static FILETIME empty = {0, 0};
    FILE_ATTRIBUTE_DATA fad;
    return GetFileAttributesExA(file, GetFileExInfoStandard, &fad) ? fad.ftLastWriteTime : empty;
}

u8 *win32_io_file_read(char *filename, u32 *file_size_out)
{
    void *hFile = INVALID_HANDLE;
    unsigned long fileSize = 0;
    unsigned long bytesRead = 0;

    u8 *buffer = 0;
    i32 attempt;

    /* Retry loop for hot-reload: file might be locked or partially written */
    for (attempt = 0; attempt < 4; ++attempt)
    {
        hFile = CreateFileA(
            filename,
            GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, /* Hot-reload safe */
            (void *)0,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
            (void *)0);

        if (hFile != INVALID_HANDLE)
        {
            break;
        }

        Sleep(5); /* Small delay, adjust as needed */
    }

    if (hFile == INVALID_HANDLE)
    {
        return (void *)0;
    }

    fileSize = GetFileSize(hFile, (void *)0);

    if (fileSize == INVALID_FILE_SIZE || fileSize == 0)
    {
        CloseHandle(hFile);
        return (void *)0;
    }

    if (fileSize <= FILE_MMAP_THRESHOLD)
    {
        /* Small file: read normally */
        buffer = (u8 *)VirtualAlloc((void *)0, fileSize + 1, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!buffer)
        {
            CloseHandle(hFile);
            return (void *)0;
        }

        if (!ReadFile(hFile, buffer, fileSize, &bytesRead, (void *)0) || bytesRead != fileSize)
        {
            VirtualFree(buffer, 0, MEM_RELEASE);
            CloseHandle(hFile);
            return (void *)0;
        }

        buffer[fileSize] = '\0'; /* Null-terminate */
    }
    else
    {
        /* Large file: memory-mapped */
        void *hMap = CreateFileMappingA(hFile, (void *)0, PAGE_READONLY, 0, 0, (void *)0);

        if (!hMap)
        {
            CloseHandle(hFile);
            return (void *)0;
        }

        buffer = (u8 *)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);

        CloseHandle(hMap);

        if (!buffer)
        {
            CloseHandle(hFile);
            return (void *)0;
        }
    }

    *file_size_out = fileSize;
    CloseHandle(hFile);
    return buffer;
}

THRIVE_API u8 win32_io_file_write(
    char *filename,
    u8 *buffer,
    u32 buffer_size)
{
    void *hFile;
    unsigned long bytes_written;

    hFile = CreateFileA(
        filename,
        GENERIC_WRITE,
        FILE_SHARE_READ,
        0,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_SEQUENTIAL_SCAN,
        0);

    if (hFile == INVALID_HANDLE)
    {
        return 0;
    }

    if (!WriteFile(hFile, buffer, buffer_size, &bytes_written, 0) ||
        bytes_written != buffer_size)
    {
        CloseHandle(hFile);
        return 0;
    }

    CloseHandle(hFile);

    return 1;
}

THRIVE_API i32 thrive_f64_to_string(
    u8 *buf,
    f64 value,
    i32 decimals, i32 width,
    u8 pad_char)
{
    u8 tmp[32];
    i32 p = 0, n = 0, neg = 0;
    i32 whole, frac, pow = 1;
    f64 x;
    i32 i;

    if (value < 0.0)
    {
        neg = 1;
        value = -value;
    }

    whole = (i32)value;
    x = value - (f64)whole;

    /* compute fraction multiplier: 10^decimals */
    for (i = 0; i < decimals; ++i)
    {
        pow *= 10;
    }

    frac = (i32)(x * (f64)pow + 0.5); /* round properly */

    /* rollover case, e.g. 1.9999 with rounding becomes 2.000 */
    if (frac >= pow)
    {
        frac -= pow;
        whole += 1;
    }

    /* convert integer part into tmp (reverse) */
    {
        i32 w = whole;

        if (w == 0)
        {
            tmp[n++] = '0';
        }

        while (w > 0)
        {
            tmp[n++] = (u8)('0' + (w % 10));
            w /= 10;
        }
    }

    /* compute total length needed */
    {
        i32 total = n + (neg ? 1 : 0) + (decimals ? (1 + decimals) : 0);
        i32 pad_needed = width - total;

        /* left padding */
        while (pad_needed-- > 0)
        {
            buf[p++] = pad_char;
        }
    }

    /* sign */
    if (neg)
    {
        buf[p++] = '-';
    }

    /* append integer part (reverse) */
    while (n--)
    {
        buf[p++] = tmp[n];
    }

    /* decimals */
    if (decimals > 0)
    {
        buf[p++] = '.';

        for (i = decimals - 1; i >= 0; --i)
        {
            i32 d = (i32)((frac / pow) % 10);

            buf[p++] = (u8)('0' + d);
            pow /= 10;

            if (i == 0)
            {
                break;
            }
        }
    }

    buf[p] = '\0';

    return p;
}
THRIVE_API void win32_io_print_ms(void *hConsole, u8 *name, u32 name_length, f64 ms, f64 ms_total)
{
    unsigned long written;

    u8 buf[128];
    u8 num[32];
    u8 *p = buf;
    u32 i;
    f64 ms_mid = 0.02;
    f64 ms_high = 0.75;

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

    {
        i32 len = thrive_f64_to_string(num, ms, 6, 0, ' ');

        for (i = 0; i < (u32)len; ++i)
        {
            *p++ = num[i];
        }
    }

    /* suffix */
    *p++ = 'm';
    *p++ = 's';

    {
        f64 percent = (ms_total > 0.0) ? (ms / ms_total * 100.0) : 0.0;
        i32 len = thrive_f64_to_string(num, percent, 2, 6, ' ');

        *p++ = ' ';

        for (i = 0; i < (u32)len; ++i)
        {
            *p++ = num[i];
        }

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
    METRIC_IO_FILE_WRITE,
    METRIC_COUNT

} win32_thrive_metrics;

/* Has to match with enum structure */
static u8 *win32_thrive_metric_names[] = {
    (u8 *)"time_io_file_read ",
    (u8 *)"time_tokenization ",
    (u8 *)"time_ast          ",
    (u8 *)"time_asm          ",
    (u8 *)"time_ast_optimized",
    (u8 *)"time_asm_optimized",
    (u8 *)"time_io_file_write"};

typedef struct win32_thrive_metric
{
    LARGE_INTEGER time_start;
    LARGE_INTEGER time_end;

} win32_thrive_metric;

/* ############################################################################
 * # Thrive Compilation
 * ############################################################################
 */
typedef struct thrive_allocator
{
    u8 *base;
    u32 size;

    thrive_token *tokens;
    u32 tokens_capacity;
    u32 tokens_size;

    thrive_ast *ast;
    u32 ast_capacity;
    u32 ast_size;

    u8 *asm_code;
    u32 asm_code_capacity;
    u32 asm_code_size;

} thrive_allocator;

THRIVE_API thrive_allocator *thrive_allocator_create(u32 source_size)
{
    thrive_allocator *arena;
    u32 max_tokens, max_ast_nodes, asm_size;
    u32 total_size;
    u8 *ptr;

    /* Estimate */
    max_tokens = source_size;
    max_ast_nodes = source_size;
    asm_size = source_size * 16;

    total_size =
        (u32)(sizeof(thrive_allocator) +
              max_tokens * sizeof(thrive_token) +
              max_ast_nodes * sizeof(thrive_ast) +
              asm_size);

    arena = (thrive_allocator *)VirtualAlloc(0, total_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    if (!arena)
    {
        return 0;
    }

    /* Subdivide */
    ptr = (u8 *)(arena + 1);

    arena->base = (u8 *)arena;
    arena->size = total_size;

    arena->tokens = (thrive_token *)ptr;
    arena->tokens_capacity = max_tokens;
    ptr += max_tokens * sizeof(thrive_token);

    arena->ast = (thrive_ast *)ptr;
    arena->ast_capacity = max_ast_nodes;
    ptr += max_ast_nodes * sizeof(thrive_ast);

    arena->asm_code = (u8 *)ptr;
    arena->asm_code_capacity = asm_size;

    return arena;
}

THRIVE_API i32 compile(char *file_name, void *hConsole, LARGE_INTEGER *freq)
{
    unsigned long written = 0;
    win32_thrive_metric metrics[METRIC_COUNT];

    u32 file_size = 0;
    u8 *file_memory;

    /* Read entire file */
    QueryPerformanceCounter(&metrics[METRIC_IO_FILE_READ].time_start);
    file_memory = win32_io_file_read(file_name, &file_size);
    QueryPerformanceCounter(&metrics[METRIC_IO_FILE_READ].time_end);

    if (!file_memory)
    {
        SetConsoleTextAttribute(hConsole, 12); /* red */
        WriteConsoleA(hConsole, "[thrive] Cannot read file!\n", 27, &written, 0);
        SetConsoleTextAttribute(hConsole, 7);
        return 1;
    }

    /* Compilation */
    {
        thrive_allocator *ta = thrive_allocator_create(file_size);

        (void)*thrive_token_type_names;

        if (!ta)
        {
            SetConsoleTextAttribute(hConsole, 12); /* red */
            WriteConsoleA(hConsole, "[thrive] Cannot allocate memory for compiler!\n", 46, &written, 0);
            SetConsoleTextAttribute(hConsole, 7);
            VirtualFree(file_memory, 0, MEM_RELEASE);

            return 1;
        }

        /* Generate Tokens */
        QueryPerformanceCounter(&metrics[METRIC_TOKENIZATION].time_start);
        thrive_tokenizer(file_memory, file_size, ta->tokens, ta->tokens_capacity, &ta->tokens_size);
        QueryPerformanceCounter(&metrics[METRIC_TOKENIZATION].time_end);

        /* Generate AST */
        QueryPerformanceCounter(&metrics[METRIC_AST].time_start);
        thrive_ast_parse(ta->tokens, ta->tokens_size, ta->ast, ta->ast_capacity, &ta->ast_size);
        QueryPerformanceCounter(&metrics[METRIC_AST].time_end);

        /* Generate Assembly */
        QueryPerformanceCounter(&metrics[METRIC_ASM].time_start);
        thrive_codegen(ta->ast, ta->ast_size, ta->asm_code, ta->asm_code_capacity, &ta->asm_code_size);
        QueryPerformanceCounter(&metrics[METRIC_ASM].time_end);

        /* Optimize AST */
        QueryPerformanceCounter(&metrics[METRIC_AST_OPTIMIZED].time_start);
        thrive_ast_optimize(ta->ast, &ta->ast_size);
        QueryPerformanceCounter(&metrics[METRIC_AST_OPTIMIZED].time_end);

        /* Generate Optimized Assembly */
        QueryPerformanceCounter(&metrics[METRIC_ASM_OPTIMIZED].time_start);
        thrive_codegen(ta->ast, ta->ast_size, ta->asm_code, ta->asm_code_capacity, &ta->asm_code_size);
        QueryPerformanceCounter(&metrics[METRIC_ASM_OPTIMIZED].time_end);

        /* Write assembly file */
        QueryPerformanceCounter(&metrics[METRIC_IO_FILE_WRITE].time_start);
        win32_io_file_write("thrive_optimized.asm", ta->asm_code, ta->asm_code_size);
        QueryPerformanceCounter(&metrics[METRIC_IO_FILE_WRITE].time_end);

        VirtualFree(file_memory, 0, MEM_RELEASE);
        VirtualFree(ta, 0, MEM_RELEASE);
    }

    /* Gather metrics */
    {
        u32 i;
        f64 metric_times[METRIC_COUNT];
        f64 metric_times_total = 0.0;

        /* Compute total */
        for (i = 0; i < METRIC_COUNT; ++i)
        {
            f64 elapsed_ms = win32_elapsed_ms(&metrics[i].time_start, &metrics[i].time_end, freq);
            metric_times[i] = elapsed_ms;
            metric_times_total += elapsed_ms;
        }

        /* Metric time */
        for (i = 0; i < METRIC_COUNT; ++i)
        {
            u8 *metric_name = win32_thrive_metric_names[i];
            win32_io_print_ms(hConsole, metric_name, thrive_string_length(metric_name), metric_times[i], metric_times_total);
        }

        /* Total time */
        win32_io_print_ms(hConsole, (u8 *)"time_total        ", 18, metric_times_total, metric_times_total);
    }

    return 0;
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
    char *file_name;

    LARGE_INTEGER freq;

    u8 conf_enable_hot_reload = 0;
    u8 conf_enable_optimized = 0;

    /* Print usage */
    if (argc < 2)
    {
        WriteConsoleA(hConsole, "[thrive] usage  : ", 18, &written, 0);
        WriteConsoleA(hConsole, argv[0], (unsigned long)thrive_string_length(argv[0]), &written, 0);
        WriteConsoleA(hConsole, " code.thrive <options>\n", 23, &written, 0);
        WriteConsoleA(hConsole, "[thrive] options:\n", 18, &written, 0);
        WriteConsoleA(hConsole, "[thrive]   --hot-reload  ; Enable hot reloading of source file\n", 63, &written, 0);
        WriteConsoleA(hConsole, "[thrive]   --optimized   ; Enable optimizations\n", 48, &written, 0);
        return 1;
    }

    /* Fetch CLI Args */
    {
        i32 i;
        for (i = 1; i < argc; ++i)
        {
            if (thrive_string_equals(argv[i], (u8 *)"--hot-reload"))
            {
                conf_enable_hot_reload = 1;
            }
            else if (thrive_string_equals(argv[i], (u8 *)"--optimized"))
            {
                conf_enable_optimized = 1;
            }
        }
    }

    (void) conf_enable_optimized;

    /* Query the CPU Frequency once */
    QueryPerformanceFrequency(&freq);

    file_name = (char *)argv[1];

    /* Compile , ... every time the source file changes */
    if (conf_enable_hot_reload)
    {

        FILETIME file_time_previous = {0};

        while (conf_enable_hot_reload)
        {
            FILETIME file_time_current = win32_io_file_mod_time(file_name);

            if (CompareFileTime(&file_time_current, &file_time_previous) != 0)
            {
                WriteConsoleA(hConsole, "[thrive] recompile\n", 19, &written, 0);
                compile(file_name, hConsole, &freq);
            }

            Sleep(5);

            file_time_previous = file_time_current;
        }
    }

    return compile(file_name, hConsole, &freq);
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
