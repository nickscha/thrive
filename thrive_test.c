
#include "thrive.h"
#include "stdio.h"

void print_token(thrive_token token)
{

    if (token.kind > THRIVE_TOKEN_KIND_INVALID)
    {
        printf("[UNKOWN]  %.*s [kind: %d]\n",
               token.end - token.start,
               token.start,
               token.kind);
    }

    switch (token.kind)
    {
    case THRIVE_TOKEN_KIND_EOF:
        printf("%-10s\n", "EOF");
        break;
    case THRIVE_TOKEN_KIND_NEW_LINE:
        printf("%-10s\n", "NEWLINE");
        break;
    case THRIVE_TOKEN_KIND_INT:
        printf("%-10s| %d\n", "INT", token.value.number);
        break;
    default:
        printf("%-10s| %.*s\n",
               thrive_token_kind_names[token.kind],
               token.end - token.start,
               token.start);
        break;
    }
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

    case THRIVE_AST_BINARY:
        printf("BINARY %s\n", thrive_token_kind_names[node->data.binary.op]);

        thrive_ast_print(node->data.binary.left, depth + 1);
        thrive_ast_print(node->data.binary.right, depth + 1);
        break;

    case THRIVE_AST_RETURN:
        printf("RETURN\n");
        thrive_ast_print(node->data.ret.expr, depth + 1);
        break;

    case THRIVE_AST_ASSIGN:
        printf("ASSIGN\n");
        thrive_ast_print(node->data.assign.left, depth + 1);
        thrive_ast_print(node->data.assign.right, depth + 1);
        break;

    case THRIVE_AST_DECL:
        printf("DECL\n");
        thrive_ast_print(node->data.decl.name, depth + 1);
        if (node->data.decl.value)
            thrive_ast_print(node->data.decl.value, depth + 1);
        break;

    case THRIVE_AST_BLOCK:
    {
        u32 i;

        printf("BLOCK\n");

        for (i = 0; i < node->data.block.count; ++i)
        {
            thrive_ast_print(node->data.block.items[i], depth + 1);
        }
        break;
    }

    default:
        printf("UNKNOWN AST\n");
        break;
    }
}

int main(void)
{
    s8 *source_code =
        "u32 a = 20 * (400 + 2)\n"
        "ret a\n";

    thrive_status status = {0};
    thrive_token token = {0};
    thrive_state state = {0};

    state.source_code = source_code;
    state.source_code_size = thrive_string_length(source_code);

    token = thrive_token_next(&state);

    print_token(token);

    while (token.kind)
    {
        token = thrive_token_next(&state);

        print_token(token);
    }

    /* Parse */
    printf("--------------------\n");
    {
        thrive_state s = {0};

        thrive_ast *ast;

        s8 *s2 =
            "u32 a = 20 * (400 + 2)\n"
            "ret a\n";

        s.source_code = s2;
        s.source_code_size = thrive_string_length(s2);

        ast = parse(&s);

        printf("=== BEFORE ===\n");
        thrive_ast_print(ast, 0);

        thrive_fold(ast);

        printf("=== AFTER ===\n");
        thrive_ast_print(ast, 0);

        /* Codegen */
        printf("--------------------\n");
        {
            gen_program(ast);
        }
    }

    return 0;
}
