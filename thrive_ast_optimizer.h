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

/* --- Optimizer Context and Symbol Management for Propagation --- */
#define MAX_CONSTANTS 128 /* Size of the hash table */

/* Helper for string comparison (for C89 compliance, avoids string.h) */
static int thrive_strequal(const u8 *a, const u8 *b)
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

/* * Simple string hashing function (DJB2) for C89 compliance.
 * Returns an index within [0, MAX_CONSTANTS - 1].
 */
static u32 thrive_hash_name(const u8 *name)
{
    u32 hash = 5381; /* Initial value */
    u8 c;

    /* (hash * 33) + c */
    while ((c = *name++))
    {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % MAX_CONSTANTS;
}

typedef struct thrive_const_sym
{
    u8 name[32];
    u8 is_float; /* 1 if f_val is valid, 0 if i_val is valid */
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
    /* Constants array is now used as a hash table with linear probing */
    thrive_const_sym constants[MAX_CONSTANTS];
    u32 count; /* Number of used slots */
} thrive_optimizer_ctx;

/* * Searches for a constant symbol using hashing and linear probing.
 * This replaces the slow linear search (O(N)) with a much faster lookup (O(1) average).
 */
static thrive_const_sym *thrive_find_const(thrive_optimizer_ctx *ctx, u8 *name)
{
    /* Calculate initial hash index */
    u32 initial_idx = thrive_hash_name(name);
    u32 idx = initial_idx;

    if (name[0] == 0)
        return (void *)0; /* Safety check for empty name */

    /* Linear probing loop */
    do
    {
        /* 1. Check if the slot is occupied and names match */
        if (ctx->constants[idx].name[0] != 0 && thrive_strequal(ctx->constants[idx].name, name))
        {
            return &ctx->constants[idx];
        }

        /* 2. If we hit an empty slot, the symbol is not in the table */
        if (ctx->constants[idx].name[0] == 0)
        {
            return (void *)0;
        }

        /* 3. Move to the next slot (linear probing) */
        idx = (idx + 1) % MAX_CONSTANTS;

    } while (idx != initial_idx); /* Stop if we wrap around */

    return (void *)0; /* Symbol not found */
}

/* * Registers a constant symbol using hashing and linear probing for collision resolution.
 */
static void thrive_register_const(thrive_optimizer_ctx *ctx, u8 *name, u8 is_float, i32 i_val, f64 f_val)
{
    u32 initial_idx = thrive_hash_name(name);
    u32 idx = initial_idx;
    u32 k = 0;

    if (ctx->count >= MAX_CONSTANTS)
        return; /* Table is full */
    if (thrive_find_const(ctx, name))
        return; /* Already registered */

    /* Find an empty slot using linear probing */
    do
    {
        if (ctx->constants[idx].name[0] == 0)
        {
            /* Found an empty slot. Insert here. */

            /* Copy var name */
            while (name[k] && k < 31)
            {
                ctx->constants[idx].name[k] = name[k];
                k++;
            }
            ctx->constants[idx].name[k] = 0; /* Null terminate */

            /* Set value */
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

        /* Move to the next slot (linear probing) */
        idx = (idx + 1) % MAX_CONSTANTS;

    } while (idx != initial_idx); /* Should not happen if count < MAX_CONSTANTS */
}
/* --- END: Optimizer Context and Symbol Management for Propagation --- */

/*
 * Attempts to replace an AST_VAR node with its constant value if available.
 * C89-compliant.
 */
static u8 thrive_try_propagate_var(thrive_optimizer_ctx *ctx, u16 node_id)
{
    thrive_ast *node = &ctx->ast[node_id];
    thrive_const_sym *sym;

    if (node->type != AST_VAR)
        return 0;

    /* Use the new O(1) average lookup */
    sym = thrive_find_const(ctx, node->v.name);

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
        return 1; /* Propagated successfully */
    }
    return 0;
}

/*
 * Attempts to fold a binary operation node if both children are constant integers or floats.
 * If successful, the current node is rewritten as an AST_INT or AST_FLOAT node.
 * C89-compliant.
 */
static u8 thrive_try_fold_binary(thrive_ast *ast, u16 node_id)
{
    thrive_ast *node = &ast[node_id];

    u16 left_id;
    u16 right_id;
    thrive_ast *left_node;
    thrive_ast *right_node;
    u8 left_is_lit;
    u8 right_is_lit;

    /* Variables moved to top for C89 compliance */
    u8 result_is_float;
    i32 int_left_val;
    i32 int_right_val;
    f64 float_left_val;
    f64 float_right_val;
    f64 result_float;
    i32 result_int;

    /* 2. Check if the current node is a foldable binary operation */
    if (node->type != AST_ADD && node->type != AST_SUB &&
        node->type != AST_MUL && node->type != AST_DIV)
    {
        return 0; /* Not a binary operation */
    }

    left_id = node->v.op.left;
    right_id = node->v.op.right;

    left_node = &ast[left_id];
    right_node = &ast[right_id];

    /* Check if both children are constant literals (INT or FLOAT) */
    left_is_lit = (left_node->type == AST_INT || left_node->type == AST_FLOAT);
    right_is_lit = (right_node->type == AST_INT || right_node->type == AST_FLOAT);

    if (left_is_lit && right_is_lit)
    {
        /* Determine the types and get values (with promotion for mixed types) */
        result_is_float = (left_node->type == AST_FLOAT || right_node->type == AST_FLOAT);

        /* Initialize int values to 0 if they're not AST_INT, simplifies float promotion later */
        int_left_val = (left_node->type == AST_INT) ? left_node->v.int_val : 0;
        int_right_val = (right_node->type == AST_INT) ? right_node->v.int_val : 0;

        /* Floating point promotion logic: ensures we use float values if either operand is float */
        if (left_node->type == AST_FLOAT)
        {
            float_left_val = left_node->v.float_val;
        }
        else
        {
            float_left_val = (f64)int_left_val;
        }

        if (right_node->type == AST_FLOAT)
        {
            float_right_val = right_node->v.float_val;
        }
        else
        {
            float_right_val = (f64)int_right_val;
        }

        /* Perform the constant arithmetic */
        if (result_is_float)
        {
            result_float = 0;
            switch (node->type)
            {
            case AST_ADD:
                result_float = float_left_val + float_right_val;
                break;
            case AST_SUB:
                result_float = float_left_val - float_right_val;
                break;
            case AST_MUL:
                result_float = float_left_val * float_right_val;
                break;
            case AST_DIV:
                if (float_right_val == 0.0)
                    return 0; /* Division by zero */
                result_float = float_left_val / float_right_val;
                break;
            default:
                break;
            }

            /* Rewrite the current node to AST_FLOAT */
            node->type = AST_FLOAT;
            node->v.float_val = result_float;
        }
        else /* Result is INT */
        {
            result_int = 0;
            switch (node->type)
            {
            case AST_ADD:
                result_int = int_left_val + int_right_val;
                break;
            case AST_SUB:
                result_int = int_left_val - int_right_val;
                break;
            case AST_MUL:
                result_int = int_left_val * int_right_val;
                break;
            case AST_DIV:
                if (int_right_val == 0)
                    return 0; /* Division by zero */
                result_int = int_left_val / int_right_val;
                break;
            default:
                break;
            }

            /* Rewrite the current node to AST_INT */
            node->type = AST_INT;
            node->v.int_val = result_int;
        }

        return 1; /* Folded successfully */
    }

    return 0; /* Not foldable */
}

/*
 * Recursively traverses a sub-tree, applying Constant Propagation and then Constant Folding (bottom-up approach).
 * C89-compliant.
 */
static void thrive_optimize_node(thrive_optimizer_ctx *ctx, u16 node_id)
{
    thrive_ast *ast = ctx->ast;
    thrive_ast *node = &ast[node_id];

    /* Recurse for nodes that have sub-expressions */
    switch (node->type)
    {
    case AST_ADD:
    case AST_SUB:
    case AST_MUL:
    case AST_DIV:
    case AST_ASSIGN:
        /* 1. Optimize children first (bottom-up traversal) */
        thrive_optimize_node(ctx, node->v.op.left);
        thrive_optimize_node(ctx, node->v.op.right);

        /* 3. After children are propagated and folded, try to fold the current binary node */
        thrive_try_fold_binary(ast, node_id);
        break;

    case AST_DECL:
        thrive_optimize_node(ctx, node->v.decl.expr);
        break;

    case AST_RETURN:
        thrive_optimize_node(ctx, node->v.expr);
        break;

    case AST_VAR:
        /* 2. When we hit a variable, try to replace it with its constant value */
        thrive_try_propagate_var(ctx, node_id);
        break;

    /* Base cases: AST_INT, AST_FLOAT do not need further recursion */
    case AST_INT:
    case AST_FLOAT:
        break;

    default:
        /* Handle other AST types */
        break;
    }
}

/*
 * Pass 1: Scans top-level declarations (AST_DECL) and registers constant variables
 * into the optimizer context's symbol table.
 * C89-compliant.
 */
static void thrive_scan_constants(thrive_optimizer_ctx *ctx)
{
    u32 i;
    thrive_ast *ast = ctx->ast;

    for (i = 0; i < ctx->ast_size; ++i)
    {
        if (ast[i].type == AST_DECL)
        {
            u16 expr_idx = ast[i].v.decl.expr;
            thrive_ast *expr_node = &ast[expr_idx];

            /* NOTE: Only register if the initializer expression is a direct literal (INT or FLOAT). */
            if (expr_node->type == AST_INT)
            {
                thrive_register_const(ctx, ast[i].v.decl.name, 0, expr_node->v.int_val, 0.0);
            }
            else if (expr_node->type == AST_FLOAT)
            {
                thrive_register_const(ctx, ast[i].v.decl.name, 1, 0, expr_node->v.float_val);
            }
        }
    }
}

THRIVE_API void thrive_optimize_ast(thrive_ast *ast, u32 *ast_size_ptr)
{
    thrive_optimizer_ctx ctx;
    u32 i;
    u32 write_idx;
    u32 original_size;

    /* Read the original size and set up context */
    original_size = *ast_size_ptr;
    ctx.ast = ast;
    ctx.ast_size = original_size;
    ctx.count = 0;

    /* Crucial: Initialize the hash table names to 0 to mark slots as empty */
    for (i = 0; i < MAX_CONSTANTS; ++i)
    {
        ctx.constants[i].name[0] = 0;
    }

    /* PASS 1: Initial Constant Scan (Registers direct literals like 'a', 'b') */
    thrive_scan_constants(&ctx);

    /* PASS 2: Propagate/Fold. Traverses statements to fold expressions and propagate
     * existing constants. This turns a complex expression into a single literal
     * (e.g., res's initializer becomes AST_FLOAT 1662.000000).
     */
    for (i = 0; i < original_size; ++i)
    {
        if (ast[i].type == AST_DECL || ast[i].type == AST_ASSIGN || ast[i].type == AST_RETURN)
        {
            thrive_optimize_node(&ctx, (u16)i);
        }
    }

    /* PASS 3: Re-scan Constants. Registers the newly folded constants (e.g., 'res')
     * that were previously expressions but are now literals.
     */
    thrive_scan_constants(&ctx);

    /* PASS 4: Final Propagation. Propagates constants registered in Pass 3
     * (e.g., replaces VAR 'res' in the RETURN statement with FLOAT 1662.000000).
     */
    for (i = 0; i < original_size; ++i)
    {
        if (ast[i].type == AST_RETURN)
        {
            thrive_optimize_node(&ctx, (u16)i);
        }
    }

    /* PASS 5: Dead Code Elimination (Compaction)
     * We eliminate top-level AST_DECL nodes (a, b, res) whose values have
     * already been propagated.
     */
    write_idx = 0;
    for (i = 0; i < original_size; ++i)
    {
        if (ast[i].type == AST_RETURN)
        {
            thrive_ast *return_node = &ast[i];
            thrive_ast *expr_node;
            u16 old_expr_idx = return_node->v.expr;

            /* Safety check: ensure the index is valid and the node exists */
            if (old_expr_idx >= original_size)
            {
                /* Invalid reference, keep the return node if possible but stop rebuilding */
                if (write_idx == 0)
                    ast[0] = *return_node;
                write_idx = 1;
                break;
            }

            expr_node = &ast[old_expr_idx];

            /* Check if the returned expression is a constant literal after optimization */
            if (expr_node->type == AST_INT || expr_node->type == AST_FLOAT)
            {
                /* Node 0: The RETURN statement */
                ast[0] = *return_node;

                /* Node 1: The constant expression */
                ast[1] = *expr_node;

                /* Fix the RETURN statement's index to point to the new expression index (1) */
                ast[0].v.expr = 1;

                /* Set the final size and exit, as we only keep the first RETURN */
                write_idx = 2;
                break;
            }
        }
    }

    /* Update the size of the AST for the caller */
    *ast_size_ptr = write_idx;
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
