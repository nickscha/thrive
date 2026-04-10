#include "thrive.h"
#include "stdio.h"

/* #############################################################################
 * # [SECTION] Print helpers
 * #############################################################################
 */
void print_token(thrive_token token)
{

    if (token.kind > THRIVE_TOKEN_KIND_INVALID)
    {
        printf("[UNKOWN]  %.*s [kind: %d]\n", token.end - token.start, token.start, token.kind);
    }

    printf("[%3d:%3d] ", token.line, token.column);

    switch (token.kind)
    {
    case THRIVE_TOKEN_KIND_EOF:
        printf("%-12s", "EOF");
        break;
    case THRIVE_TOKEN_KIND_NEW_LINE:
        printf("%-12s", "NEWLINE");
        break;
    case THRIVE_TOKEN_KIND_INT:
        printf("%-12s| %d", "INT", token.value.number);
        break;
    default:
        printf("%-12s| %.*s", thrive_token_kind_names[token.kind], token.end - token.start, token.start);
        break;
    }

    printf("\n");
}

THRIVE_API void thrive_print_indent(u32 depth)
{
    u32 i;
    for (i = 0; i < depth; ++i)
    {
        printf("  ");
    }
}

THRIVE_API void thrive_ast_print(thrive_ast *node, u32 depth)
{
    if (!node)
    {
        thrive_print_indent(depth);
        printf("(null)\n");
        return;
    }

    thrive_print_indent(depth);

    switch (node->kind)
    {
    case THRIVE_AST_INT:
        printf("INT %u\n", node->data.int_value);
        break;

    case THRIVE_AST_NAME:
        printf("NAME %.*s\n", node->data.name.length, node->data.name.start);
        break;

    case THRIVE_AST_STRING:
    {
        printf("STRING \"%.*s\"\n",
               node->data.string_lit.length,
               node->data.string_lit.start);
        break;
    }

    case THRIVE_AST_BINARY:
        printf("BINARY %s\n", thrive_token_kind_names[node->data.binary.op]);

        thrive_ast_print(node->data.binary.left, depth + 1);
        thrive_ast_print(node->data.binary.right, depth + 1);
        break;

    case THRIVE_AST_UNARY:
    {
        if (node->data.unary.op == THRIVE_TOKEN_KIND_INC)
        {
            printf("POST_INC\n");
            thrive_ast_print(node->data.unary.expr, depth + 1);
        }
        else if (node->data.unary.op == THRIVE_TOKEN_KIND_DEC)
        {
            printf("POST_DEC\n");
            thrive_ast_print(node->data.unary.expr, depth + 1);
        }
        else
        {
            printf("UNARY %s\n", thrive_token_kind_names[node->data.unary.op]);
            thrive_ast_print(node->data.unary.expr, depth + 1);
        }
        break;
    }

    case THRIVE_AST_TERNARY:
        printf("TERNARY\n");

        thrive_ast_print(node->data.ternary.cond, depth + 1);
        thrive_ast_print(node->data.ternary.then_expr, depth + 1);
        thrive_ast_print(node->data.ternary.else_expr, depth + 1);
        break;

    case THRIVE_AST_IF:
    {
        printf("IF\n");

        thrive_print_indent(depth + 1);
        printf("COND\n");
        thrive_ast_print(node->data.if_stmt.cond, depth + 2);

        thrive_print_indent(depth + 1);
        printf("THEN\n");
        thrive_ast_print(node->data.if_stmt.then_branch, depth + 2);

        if (node->data.if_stmt.else_branch)
        {
            thrive_print_indent(depth + 1);
            printf("ELSE\n");
            thrive_ast_print(node->data.if_stmt.else_branch, depth + 2);
        }

        break;
    }

    case THRIVE_AST_FOR:
    {
        printf("FOR\n");

        thrive_print_indent(depth + 1);
        printf("INIT\n");
        thrive_ast_print(node->data.for_loop.init, depth + 2);

        thrive_print_indent(depth + 1);
        printf("COND\n");
        thrive_ast_print(node->data.for_loop.cond, depth + 2);

        thrive_print_indent(depth + 1);
        printf("STEP\n");
        thrive_ast_print(node->data.for_loop.step, depth + 2);

        thrive_print_indent(depth + 1);
        printf("BODY\n");
        thrive_ast_print(node->data.for_loop.body, depth + 2);
        break;
    }

    case THRIVE_AST_ADDR_OF:
        printf("ADDRESS_OF (&)\n");
        thrive_ast_print(node->data.unary.expr, depth + 1);
        break;

    case THRIVE_AST_DEREF:
        printf("DEREFERENCE (*)\n");
        thrive_ast_print(node->data.unary.expr, depth + 1);
        break;

    case THRIVE_AST_RETURN:
        printf("RETURN\n");
        thrive_ast_print(node->data.ret.expr, depth + 1);
        break;

    case THRIVE_AST_BREAK:
        printf("BREAK\n");
        break;

    case THRIVE_AST_CONTINUE:
        printf("CONTINUE\n");
        break;

    case THRIVE_AST_ASSIGN:
        printf("ASSIGN\n");
        thrive_ast_print(node->data.assign.left, depth + 1);
        thrive_ast_print(node->data.assign.right, depth + 1);
        break;

    case THRIVE_AST_DECL:
    {
        if (node->data.decl.is_array)
        {
            printf("DECL ARRAY [size: %u]\n", node->data.decl.array_size);
        }
        else
        {
            printf("DECL\n");
        }

        thrive_ast_print(node->data.decl.name, depth + 1);

        if (node->data.decl.value)
        {
            thrive_ast_print(node->data.decl.value, depth + 1);
        }
        break;
    }

    case THRIVE_AST_BLOCK:
    {
        thrive_ast *curr = node->data.block.body;
        printf("BLOCK\n");
        while (curr)
        {
            thrive_ast_print(curr, depth + 1);
            curr = curr->next;
        }
        break;
    }

    case THRIVE_AST_FUNC_DECL:
    {
        thrive_ast *curr;
        printf("FUNC_DECL %.*s\n",
               node->data.func_decl.name->data.name.length,
               node->data.func_decl.name->data.name.start);

        curr = node->data.func_decl.params;
        if (curr)
        {
            thrive_print_indent(depth + 1);
            printf("PARAMS:\n");
            while (curr)
            {
                thrive_ast_print(curr, depth + 2);
                curr = curr->next;
            }
        }

        thrive_print_indent(depth + 1);
        printf("BODY:\n");
        thrive_ast_print(node->data.func_decl.body, depth + 2);
        break;
    }

    case THRIVE_AST_FUNC_CALL:
    {
        thrive_ast *curr = node->data.func_call.args;
        printf("FUNC_CALL %.*s\n",
               node->data.func_call.name->data.name.length,
               node->data.func_call.name->data.name.start);

        if (curr)
        {
            thrive_print_indent(depth + 1);
            printf("ARGS:\n");
            while (curr)
            {
                thrive_ast_print(curr, depth + 2);
                curr = curr->next;
            }
        }
        break;
    }

    case THRIVE_AST_EXT_DECL:
    {
        thrive_ast *curr = node->data.ext_decl.params;

        printf("EXT_DECL %.*s\n",
               node->data.ext_decl.name->data.name.length,
               node->data.ext_decl.name->data.name.start);

        if (curr)
        {
            thrive_print_indent(depth + 1);
            printf("PARAMS:\n");
            while (curr)
            {
                thrive_ast_print(curr, depth + 2);
                curr = curr->next;
            }
        }
        break;
    }

    case THRIVE_AST_ARRAY_ACCESS:
    {
        printf("ARRAY_ACCESS\n");

        thrive_print_indent(depth + 1);
        printf("BASE:\n");
        thrive_ast_print(node->data.array_access.left, depth + 2);

        thrive_print_indent(depth + 1);
        printf("INDEX:\n");
        thrive_ast_print(node->data.array_access.index, depth + 2);
        break;
    }

    default:
        printf("UNKNOWN AST\n");
        break;
    }
}