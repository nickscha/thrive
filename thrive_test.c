
#include "thrive.h"
#include "stdio.h"

/* #############################################################################
 * # [SECTION] Codegen
 * #############################################################################
 */
#define THRIVE_MAX_VARS 256

typedef struct
{
    s8 *start;
    u32 length;
    i32 offset; /* stack offset (negative from rbp) */
} thrive_var;

static thrive_var vars[THRIVE_MAX_VARS];
static u32 var_count = 0;
static i32 stack_offset = 0;

static int label_id = 0;

i32 new_label(void)
{
    return label_id++;
}

thrive_var *find_var(s8 *start, u32 length)
{
    u32 i;

    for (i = 0; i < var_count; ++i)
    {
        if (vars[i].length == length && thrive_string_equals(vars[i].start, start, length))
        {
            return &vars[i];
        }
    }

    return 0;
}

thrive_var *add_var(s8 *start, u32 length)
{
    thrive_var *v = &vars[var_count++];

    stack_offset -= 8; /* 8 bytes per variable */

    v->start = start;
    v->length = length;
    v->offset = stack_offset;

    return v;
}

void gen_expr(thrive_ast *node);

void gen_expr(thrive_ast *node)
{
    switch (node->kind)
    {
    case THRIVE_AST_INT:
        printf("    mov rax, %u\n", node->data.int_value);
        break;
    case THRIVE_AST_NAME:
    {
        thrive_var *v = find_var(
            node->data.name.start,
            node->data.name.length);

        printf("    mov rax, [rbp%d]\n", v->offset);
        break;
    }
    case THRIVE_AST_BINARY:
    {
        gen_expr(node->data.binary.left);
        printf("    push rax\n");

        gen_expr(node->data.binary.right);
        printf("    pop rbx\n");

        switch (node->data.binary.op)
        {
        case THRIVE_TOKEN_KIND_ADD:
            printf("    add rax, rbx\n");
            break;

        case THRIVE_TOKEN_KIND_SUB:
            printf("    sub rbx, rax\n");
            printf("    mov rax, rbx\n");
            break;

        case THRIVE_TOKEN_KIND_MUL:
            printf("    imul rax, rbx\n");
            break;

        case THRIVE_TOKEN_KIND_DIV:
            printf("    mov rdx, 0\n");
            printf("    mov rcx, rax\n");
            printf("    mov rax, rbx\n");
            printf("    idiv rcx\n");
            break;

        case THRIVE_TOKEN_KIND_LT:
            printf("    cmp rbx, rax\n");
            printf("    setl al\n");
            printf("    movzx rax, al\n");
            break;

        case THRIVE_TOKEN_KIND_GT:
            printf("    cmp rbx, rax\n");
            printf("    setg al\n");
            printf("    movzx rax, al\n");
            break;

        case THRIVE_TOKEN_KIND_LT_EQUALS:
            printf("    cmp rbx, rax\n");
            printf("    setle al\n");
            printf("    movzx rax, al\n");
            break;

        case THRIVE_TOKEN_KIND_GT_EQUALS:
            printf("    cmp rbx, rax\n");
            printf("    setge al\n");
            printf("    movzx rax, al\n");
            break;

        case THRIVE_TOKEN_KIND_EQUALS:
            printf("    cmp rbx, rax\n");
            printf("    sete al\n");
            printf("    movzx rax, al\n");
            break;

        case THRIVE_TOKEN_KIND_NOT_EQUALS:
            printf("    cmp rbx, rax\n");
            printf("    setne al\n");
            printf("    movzx rax, al\n");
            break;

        case THRIVE_TOKEN_KIND_AND_BITWISE:
            printf("    and rax, rbx\n");
            break;

        case THRIVE_TOKEN_KIND_OR_BITWISE:
            printf("    or rax, rbx\n");
            break;

        case THRIVE_TOKEN_KIND_AND_LOGICAL:
        {
            int l_false = new_label();
            int l_end = new_label();

            gen_expr(node->data.binary.left);
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", l_false);

            gen_expr(node->data.binary.right);
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", l_false);

            printf("    mov rax, 1\n");
            printf("    jmp .L%d\n", l_end);

            printf(".L%d:\n", l_false);
            printf("    mov rax, 0\n");

            printf(".L%d:\n", l_end);
            break;
        }

        case THRIVE_TOKEN_KIND_OR_LOGICAL:
        {
            int l_true = new_label();
            int l_end = new_label();

            gen_expr(node->data.binary.left);
            printf("    cmp rax, 0\n");
            printf("    jne .L%d\n", l_true);

            gen_expr(node->data.binary.right);
            printf("    cmp rax, 0\n");
            printf("    jne .L%d\n", l_true);

            printf("    mov rax, 0\n");
            printf("    jmp .L%d\n", l_end);

            printf(".L%d:\n", l_true);
            printf("    mov rax, 1\n");

            printf(".L%d:\n", l_end);
            break;
        }

        default:
            break;
        }
        break;
    }

    case THRIVE_AST_UNARY:
    {
        if (node->data.unary.op == THRIVE_TOKEN_KIND_INC || node->data.unary.op == THRIVE_TOKEN_KIND_DEC)
        {
            thrive_ast *name_node = node->data.unary.expr;
            thrive_var *v = find_var(name_node->data.name.start, name_node->data.name.length);

            if (node->data.unary.op == THRIVE_TOKEN_KIND_INC)
            {
                printf("    inc qword [rbp%d]\n", v->offset);
            }
            else
            {
                printf("    dec qword [rbp%d]\n", v->offset);
            }
            printf("    mov rax, [rbp%d]\n", v->offset);
        }
        else
        {
            gen_expr(node->data.unary.expr);

            switch (node->data.unary.op)
            {
            case THRIVE_TOKEN_KIND_SUB:
                printf("    neg rax\n");
                break;
            case THRIVE_TOKEN_KIND_ADD:
                /* no-op */
                break;
            case THRIVE_TOKEN_KIND_NEGATE:
                printf("    cmp rax, 0\n");
                printf("    sete al\n");
                printf("    movzx rax, al\n");
                break;
            default:
                break;
            }
        }

        break;
    }

    case THRIVE_AST_TERNARY:
    {
        i32 l_else = new_label();
        i32 l_end = new_label();

        /* evaluate condition */
        gen_expr(node->data.ternary.cond);

        printf("    cmp rax, 0\n");
        printf("    je .L%d\n", l_else);

        /* then branch */
        gen_expr(node->data.ternary.then_expr);
        printf("    jmp .L%d\n", l_end);

        /* else branch */
        printf(".L%d:\n", l_else);
        gen_expr(node->data.ternary.else_expr);

        printf(".L%d:\n", l_end);
        break;
    }

    case THRIVE_AST_ASSIGN:
    {
        thrive_ast *name = node->data.assign.left;
        thrive_ast *value = node->data.assign.right;

        gen_expr(value);

        thrive_var *v = find_var(name->data.name.start, name->data.name.length);

        printf("    mov [rbp%d], rax\n", v->offset);

        break;
    }

    case THRIVE_AST_ADDR_OF:
    {
        thrive_ast *target = node->data.unary.expr;
        thrive_var *v = find_var(target->data.name.start, target->data.name.length);
        printf("    lea rax, [rbp%d]\n", v->offset);
        break;
    }

    case THRIVE_AST_DEREF:
    {
        /* *ptr -> Treat the value in the variable as an address */
        gen_expr(node->data.unary.expr);

        printf("    mov rax, [rax]\n");
        break;
    }

    default:
        break;
    }
}

void gen_stmt(thrive_ast *node)
{
    switch (node->kind)
    {
    case THRIVE_AST_DECL:
    {
        thrive_ast *name = node->data.decl.name;
        thrive_ast *value = node->data.decl.value;

        thrive_var *v = add_var(name->data.name.start, name->data.name.length);

        if (value)
        {
            gen_expr(value);
            printf("    mov [rbp%d], rax\n", v->offset);
        }

        break;
    }
    case THRIVE_AST_RETURN:
    {
        gen_expr(node->data.ret.expr);

        printf("    mov rcx, rax\n"); /* argument */
        printf("    call ExitProcess\n");

        break;
    }
    case THRIVE_AST_IF:
    {
        int l_else = new_label();
        int l_end = new_label();

        /* condition */
        gen_expr(node->data.if_stmt.cond);

        printf("    cmp rax, 0\n");

        if (node->data.if_stmt.else_branch)
            printf("    je .L%d\n", l_else);
        else
            printf("    je .L%d\n", l_end);

        /* then */
        gen_stmt(node->data.if_stmt.then_branch);

        if (node->data.if_stmt.else_branch)
        {
            printf("    jmp .L%d\n", l_end);

            printf(".L%d:\n", l_else);
            gen_stmt(node->data.if_stmt.else_branch);
        }

        printf(".L%d:\n", l_end);

        break;
    }
    case THRIVE_AST_FOR:
    {
        i32 l_start = new_label();
        i32 l_end = new_label();

        if (node->data.for_loop.init)
        {
            gen_expr(node->data.for_loop.init);
        }

        printf(".L%d:\n", l_start);

        /* 2. Condition: i < 10 */
        if (node->data.for_loop.cond)
        {
            gen_expr(node->data.for_loop.cond);
            printf("    cmp rax, 0\n");
            printf("    je .L%d\n", l_end);
        }

        if (node->data.for_loop.body)
        {
            gen_stmt(node->data.for_loop.body);
        }

        if (node->data.for_loop.step)
        {
            gen_expr(node->data.for_loop.step);
        }

        printf("    jmp .L%d\n", l_start);
        printf(".L%d:\n", l_end);
        break;
    }
    case THRIVE_AST_BLOCK:
    {
        u32 i;

        for (i = 0; i < node->data.block.count; ++i)
        {
            gen_stmt(node->data.block.items[i]);
        }
        break;
    }
    default:
        gen_expr(node);
        break;
    }
}

void gen_program(thrive_ast *node)
{
    u32 i;

    printf("default rel\n");
    printf("extern ExitProcess\n\n");

    printf("section .text\n");
    printf("global main\n\n");

    printf("main:\n");

    /* prologue */
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, 32\n"); /* shadow space */

    for (i = 0; i < node->data.block.count; ++i)
    {
        gen_stmt(node->data.block.items[i]);
    }
}

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
        printf("%-10s", "EOF");
        break;
    case THRIVE_TOKEN_KIND_NEW_LINE:
        printf("%-10s", "NEWLINE");
        break;
    case THRIVE_TOKEN_KIND_INT:
        printf("%-10s| %d", "INT", token.value.number);
        break;
    default:
        printf("%-10s| %.*s", thrive_token_kind_names[token.kind], token.end - token.start, token.start);
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

/* #############################################################################
 * # [SECTION] Testing
 * #############################################################################
 */
int main(void)
{

    s8 *source_code =
        "u32 i = 1\n"
        "u32 b = &i\n"
        "u32 c = *&i\n"
        "\n"
        "for(i = 0 : i < 10 : i++) {\n"
        " i += 1\n"
        "}\n"
        "\n"
        "ret i\n";

    /*
    s8 *source_code =
        "; this is a line comment\n"
        "u32 a\n"
        "a = 20 * (400 + 2) ;another comment 1 * 2\n"
        "ret a\n";

    s8 *source_code =
        "u32 a = 10\n"
        "if (a > 5) {\n"
        " a = 1\n"
        " a = 2\n"
        "} else {\n"
        " a = 3\n"
        "}\n"
        "ret a\n";
    */

    thrive_state state = {0};

    state.line = 1;
    state.column = 1;
    state.source_code = source_code;
    state.source_code_size = thrive_string_length(source_code);

    printf("--------------------\n");
    printf("%s", source_code);
    printf("--------------------\n");
    printf("[size] thrive_status    = %10d\n", (u32)sizeof(thrive_status));
    printf("[size] thrive_token     = %10d\n", (u32)sizeof(thrive_token));
    printf("[size] thrive_state     = %10d\n", (u32)sizeof(thrive_state));
    printf("[size] thrive_ast_block = %10d\n", (u32)sizeof(thrive_ast_block));
    printf("[size] thrive_ast       = %10d\n", (u32)sizeof(thrive_ast));
    printf("[size] thrive_ast_pool  = %10d\n", (u32)sizeof(thrive_ast_pool));
    printf("--------------------\n");

    thrive_token_next(&state);

    print_token(state.current);

    while (state.current.kind)
    {
        thrive_token_next(&state);

        print_token(state.current);
    }

    /* Parse */
    printf("--------------------\n");
    {
        thrive_state s = {0};

        thrive_ast *ast;

        /*
        s8 *s2 =
            "; this is a line comment          \n"
            "u32 a   = 42                      \n"
            "u32 b   = 27                      \n"
            "u32 c   = 1                       \n"
            "u32 res = a * c + b * 10 * (2 + 4)\n"
            "ret res                           \n";
        */

        s.line = 1;
        s.column = 1;
        s.source_code = source_code;
        s.source_code_size = thrive_string_length(source_code);

        ast = thrive_ast_parse(&s);

        if (s.status.type != THRIVE_STATUS_OK)
        {
            printf("[ERROR %d:%d] code: %d message: %s\n", s.status.line, s.status.column, s.status.type, s.status.message);
            return 1;
        }

        printf("=== BEFORE ===\n");
        thrive_ast_print(ast, 0);

        ast = thrive_ast_fold(ast);

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
