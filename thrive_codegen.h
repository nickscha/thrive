/* thrive.h - v0.1 - public domain data structures - nickscha 2025

A C89 standard compliant, nostdlib (no C Standard Library) Low Level Programming Language inbetween Assembly and C (THRIVE).

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#ifndef THRIVE_CODEGEN_H
#define THRIVE_CODEGEN_H

#include "thrive.h"
#include "thrive_ast.h"

/*
 * Windows x64 NASM Code Generator
 * - Output: Writes to a u8 buffer
 * - Model: Global Static Data (No Stack Frames for locals)
 * - Sections: .data (constants), .bss (vars), .text (code)
 * - NOSTDLIB: No includes from standard library
 * - NO 64-BIT TYPES: Uses only u32/i32 internally
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

THRIVE_API void thrive_cg_emit_str(thrive_codegen_ctx *ctx, const char *s)
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

    for (j = i - 1; j >= 0; j--)
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
        thrive_cg_emit_char(ctx, nibble + '0');
    else
        thrive_cg_emit_char(ctx, nibble - 10 + 'A');
}

THRIVE_API void thrive_cg_emit_hex_u32_full(thrive_codegen_ctx *ctx, u32 v)
{
    i32 i;
    for (i = 28; i >= 0; i -= 4)
    {
        thrive_cg_emit_hex_digit(ctx, (u8)((v >> i) & 0xF));
    }
}

/* ---- Symbol Management ---- */

THRIVE_API int thrive_streq(const u8 *a, const u8 *b)
{
    while (*a && *b)
    {
        if (*a != *b)
            return 0;
        a++;
        b++;
    }
    return (*a == *b);
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
        return; /* Already exists */
    if (ctx->globals_count >= MAX_GLOBALS)
        return;

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

/* ---- Assembly Emitters ---- */

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
        ctx.buf[0] = 0;

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
    /* Note: We must align stack if we call functions, but ExitProcess is fine.
       If we called C functions we'd need 'sub rsp, 40' (shadow space + align) */
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

#endif /* THRIVE_CODEGEN_H */

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
