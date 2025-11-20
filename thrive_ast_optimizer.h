/* thrive.h - v0.1 - public domain data structures - nickscha 2025

A C89 standard compliant, nostdlib (no C Standard Library) Low Level Programming Language inbetween Assembly and C (THRIVE).

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#ifndef THRIVE_AST_OPTIMIZER_H
#define THRIVE_AST_OPTIMIZER_H

#include "thrive.h"
#include "thrive_ast.h"

#define MAX_CONSTANTS 128
#define MAX_AST_NODES 1024

THRIVE_API int thrive_strequal(u8 *a, u8 *b)
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
        return (void *)0;

    do
    {
        if (ctx->constants[idx].name[0] != 0 && thrive_strequal(ctx->constants[idx].name, name))
            return &ctx->constants[idx];
        if (ctx->constants[idx].name[0] == 0)
            return (void *)0;
        idx = (idx + 1) % MAX_CONSTANTS;
    } while (idx != initial_idx);

    return (void *)0;
}

THRIVE_API void thrive_ast_register_const(thrive_optimizer_ctx *ctx, u8 *name, u8 is_float, i32 i_val, f64 f_val)
{
    u32 initial_idx = thrive_hash_name(name);
    u32 idx = initial_idx;
    u32 k = 0;

    if (ctx->count >= MAX_CONSTANTS)
        return;
    if (thrive_ast_find_const(ctx, name))
        return;

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
                ctx->constants[idx].val.f_val = f_val;
            else
                ctx->constants[idx].val.i_val = i_val;
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
        return 0;

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
        return;
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
                thrive_ast_register_const(ctx, ast[i].v.decl.name, 0, expr_node->v.int_val, 0.0);
            else if (expr_node->type == AST_FLOAT)
                thrive_ast_register_const(ctx, ast[i].v.decl.name, 1, 0, expr_node->v.float_val);
        }
    }
}

/* --- General Dead Code Elimination (Mark-Sweep-Compact) --- */

THRIVE_API void thrive_ast_mark_alive(thrive_ast *ast, u8 *alive_map, u16 node_id)
{
    thrive_ast *node;

    /* If already marked, stop recursion */
    if (alive_map[node_id])
        return;

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
        /* NOTE: If you support non-constant vars, you'd add a check here to mark DECLs that aren't in the constant table */
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
            thrive_ast_optimize_node(&ctx, (u16)i);
    }

    /* Pass 3 & 4: Rescan and Propagate (Second Pass for deep folding) */
    thrive_ast_scan_constants(&ctx);
    for (i = 0; i < ctx.ast_size; ++i)
    {
        if (ast[i].type == AST_RETURN)
        { /* Primarily target returns for final propagation */
            thrive_ast_optimize_node(&ctx, (u16)i);
        }
    }

    /* Pass 5: General Dead Code Elimination */
    thrive_ast_compact(ast, ast_size_ptr);
}

#endif /* THRIVE_AST_OPTIMIZER_H */

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
