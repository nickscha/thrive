/* thrive.h - v0.1 - public domain data structures - nickscha 2025

A C89 standard compliant, nostdlib (no C Standard Library) Low Level Programming Language inbetween Assembly and C (THRIVE).

This Test class defines cases to verify that we don't break the excepted behaviours in the future upon changes.

LICENSE

  Placed in the public domain and also MIT licensed.
  See end of file for detailed license information.

*/
#include "../thrive.h"
#include "../thrive_tokenizer.h"
#include "../thrive_ast.h"
#include "../thrive_ast_optimizer.h"
#include "../thrive_codegen.h"
#include "../deps/test.h" /* Simple Testing framework    */

#include "stdio.h"

#define TOKENS_CAPACITY 1024
#define AST_CAPACITY 1024
#define CODE_CAPACITY 8192

static void thrive_print_tokens(thrive_token *tokens, u32 tokens_size)
{
  u32 i;

  for (i = 0; i < tokens_size; ++i)
  {
    thrive_token t = tokens[i];
    thrive_token_type tt = t.type;

    if (tt == THRIVE_TOKEN_VAR)
    {
      printf("[hgl-token] %-25s = %s\n", thrive_token_type_names[tt], t.value.name);
    }
    else if (tt == THRIVE_TOKEN_INTEGER)
    {
      printf("[hgl-token] %-25s = %d\n", thrive_token_type_names[tt], t.value.integer);
    }
    else if (tt == THRIVE_TOKEN_FLOAT)
    {
      printf("[hgl-token] %-25s = %10.6f\n", thrive_token_type_names[tt], t.value.floating);
    }
    else
    {
      printf("[hgl-token] %-25s =\n", thrive_token_type_names[tt]);
    }
  }
}

static void thrive_print_ast(thrive_ast *ast, u32 node, u32 depth)
{
  u32 i;

  printf("[hgl-ast] ");

  for (i = 0; i < depth; ++i)
  {
    printf("  "); /* two-space indent */
  }

  switch (ast[node].type)
  {

  case AST_INT:
    printf("INT %d\n", ast[node].v.int_val);
    return;

  case AST_FLOAT:
    printf("FLOAT %f\n", ast[node].v.float_val);
    return;

  case AST_VAR:
    printf("VAR %s\n", ast[node].v.name);
    return;

  case AST_ADD:
    printf("ADD\n");
    thrive_print_ast(ast, ast[node].v.op.left, depth + 1);
    thrive_print_ast(ast, ast[node].v.op.right, depth + 1);
    return;

  case AST_SUB:
    printf("SUB\n");
    thrive_print_ast(ast, ast[node].v.op.left, depth + 1);
    thrive_print_ast(ast, ast[node].v.op.right, depth + 1);
    return;

  case AST_MUL:
    printf("MUL\n");
    thrive_print_ast(ast, ast[node].v.op.left, depth + 1);
    thrive_print_ast(ast, ast[node].v.op.right, depth + 1);
    return;

  case AST_DIV:
    printf("DIV\n");
    thrive_print_ast(ast, ast[node].v.op.left, depth + 1);
    thrive_print_ast(ast, ast[node].v.op.right, depth + 1);
    return;

  case AST_ASSIGN:
    printf("ASSIGN\n");
    thrive_print_ast(ast, ast[node].v.op.left, depth + 1);
    thrive_print_ast(ast, ast[node].v.op.right, depth + 1);
    return;

  case AST_DECL:
    printf("DECL %s\n", ast[node].v.decl.name);
    thrive_print_ast(ast, ast[node].v.decl.expr, depth + 1);
    return;

  case AST_RETURN:
    printf("RETURN\n");
    thrive_print_ast(ast, ast[node].v.expr, depth + 1);
    return;

  default:
    printf("UNKNOWN\n");
    return;
  }
}

static void thrive_test_compiler(void)
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

  /* Generate Tokens */
  assert(thrive_tokenizer(code, code_size, tokens, TOKENS_CAPACITY, &tokens_size));
  assert(tokens_size == 25);
  thrive_print_tokens(tokens, tokens_size);

  /* Generate AST */
  thrive_ast_parse(tokens, tokens_size, ast, AST_CAPACITY, &ast_size);
  assert(ast_size == 16);

  {
    u32 i;

    for (i = 0; i < ast_size; ++i)
    {
      if (ast[i].type == AST_DECL ||
          ast[i].type == AST_ASSIGN ||
          ast[i].type == AST_RETURN)
      {
        thrive_print_ast(ast, i, 0);
      }
    }
  }

  /* Generate Assembly */
  thrive_codegen(ast, ast_size, code_asm, CODE_CAPACITY, &code_asm_size);
  assert(code_asm_size > 0);
  printf("[hgl-asm-x86_64]\n%s", code_asm);

  /* optimize ast */
  thrive_ast_optimize(ast, &ast_size);
  assert(ast_size == 2);
  assert(ast[0].type == AST_RETURN);
  assert(ast[1].type == AST_FLOAT);
  assert_equalsd(ast[1].v.float_val, 1662.0, 1e-6);

  {
    u32 i;

    for (i = 0; i < ast_size; ++i)
    {
      if (ast[i].type == AST_DECL ||
          ast[i].type == AST_ASSIGN ||
          ast[i].type == AST_RETURN)
      {
        thrive_print_ast(ast, i, 0);
      }
    }
  }

  thrive_codegen(ast, ast_size, code_asm, CODE_CAPACITY, &code_asm_size);
  assert(code_asm_size > 0);
  printf("[hgl-asm-x86_64-optimized]\n%s", code_asm);
}

int main(void)
{
  thrive_test_compiler();
  return 0;
}

/*
   -----------------------------------------------------------------------------
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
