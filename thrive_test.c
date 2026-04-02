
#include "thrive.h"
#include "stdio.h"
#include "stdlib.h"

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
static i32 in_function = 0;

static i32 label_id = 0;
static i32 current_break_label = -1;
static i32 current_continue_label = -1;

typedef struct
{
    s8 *start;
    u32 length;
} thrive_string_data;

static thrive_string_data string_pool[256];
static u32 string_count = 0;

void reset_locals(void)
{
    var_count = 0;
    stack_offset = 0;
}

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
            printf("    xor rdx, rdx\n");
            printf("    mov rcx, rax\n");
            printf("    pop rax\n");
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
            i32 l_false = new_label();
            i32 l_end = new_label();

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
            i32 l_true = new_label();
            i32 l_end = new_label();

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
        thrive_ast *left = node->data.assign.left;
        thrive_ast *right = node->data.assign.right;

        if (left->kind == THRIVE_AST_DEREF)
        {
            /* Case: *p = 10 */
            gen_expr(right);                 /* rax = 10 */
            printf("    push rax\n");        /* save 10 */
            gen_expr(left->data.unary.expr); /* rax = address in p */
            printf("    pop rbx\n");         /* rbx = 10 */
            printf("    mov [rax], rbx\n");  /* store 10 at address in rax */
        }
        else
        {
            /* Case: i = 10 */
            gen_expr(right);
            thrive_var *v = find_var(left->data.name.start, left->data.name.length);
            printf("    mov [rbp%d], rax\n", v->offset);
        }
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

    case THRIVE_AST_BREAK:
    {
        if (current_break_label == -1)
        {
            /* Error: Break used outside of loop */
            return;
        }
        printf("    jmp .L%d\n", current_break_label);
        break;
    }

    case THRIVE_AST_CONTINUE:
    {
        if (current_continue_label == -1)
        {
            /* Error: Continue used outside of loop */
            return;
        }
        printf("    jmp .L%d\n", current_continue_label);
        break;
    }

    case THRIVE_AST_FUNC_CALL:
    {
        u32 i;
        s8 *arg_regs[] = {"rcx", "rdx", "r8", "r9"};
        u32 arg_count = node->data.func_call.arg_count;

        /* 1. Evaluate and push all arguments */
        for (i = 0; i < arg_count; ++i)
        {
            gen_expr(node->data.func_call.args[i]);
            printf("    push rax\n");
        }

        /* 2. Pop up to the first 4 into registers */
        u32 reg_args = arg_count > 4 ? 4 : arg_count;
        for (i = reg_args; i > 0; --i)
        {
            printf("    pop %s\n", arg_regs[i - 1]);
        }

        /* 3. The 5th+ arguments are already on the stack.
           We must add 32 bytes of shadow space for the called function. */
        printf("    sub rsp, 32\n");

        /* TODO: To be perfectly safe, you must calculate total stack depth here
           and ensure (rsp & 15) == 0 before the call. */

        printf("    call %.*s\n",
               node->data.func_call.name->data.name.length,
               node->data.func_call.name->data.name.start);

        /* 4. Clean up shadow space AND stack arguments */
        u32 stack_cleanup = 32;
        if (arg_count > 4)
        {
            stack_cleanup += (arg_count - 4) * 8;
        }
        printf("    add rsp, %u\n", stack_cleanup);

        break;
    }

    case THRIVE_AST_STRING:
    {
        u32 id = string_count++;
        string_pool[id].start = node->data.string_lit.start;
        string_pool[id].length = node->data.string_lit.length;

        printf("    lea rax, [rel .STR%d]\n", id);
        break;
    }

    case THRIVE_TOKEN_KIND_LSHIFT:
    {
        printf("    mov rcx, rax\n");
        printf("    shl rbx, cl\n");
        printf("    mov rax, rbx\n");
        break;
    }
    case THRIVE_TOKEN_KIND_RSHIFT:
    {
        printf("    mov rcx, rax\n");
        printf("    shr rbx, cl\n");
        printf("    mov rax, rbx\n");
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
        i32 start_label = new_label();
        i32 step_label = new_label();
        i32 end_label = new_label();

        /* Save parent loop context */
        i32 old_break = current_break_label;
        i32 old_continue = current_continue_label;

        /* Set current loop context for children */
        current_break_label = end_label;
        current_continue_label = step_label;

        /* 1. Init */
        gen_expr(node->data.for_loop.init);

        printf(".L%d:\n", start_label);

        /* 2. Condition */
        gen_expr(node->data.for_loop.cond);
        printf("    test rax, rax\n");
        printf("    jz .L%d\n", end_label);

        /* 3. Body */
        gen_stmt(node->data.for_loop.body);

        /* 4. Step (Continue jumps here!) */
        printf(".L%d:\n", step_label);
        gen_expr(node->data.for_loop.step);
        printf("    jmp .L%d\n", start_label);

        /* 5. End (Break jumps here!) */
        printf(".L%d:\n", end_label);

        /* Restore parent context */
        current_break_label = old_break;
        current_continue_label = old_continue;
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

    case THRIVE_AST_FUNC_DECL:
    {
        u32 i;
        s8 *arg_regs[] = {"rcx", "rdx", "r8", "r9"};

        printf("\n%.*s:\n", node->data.func_decl.name->data.name.length, node->data.func_decl.name->data.name.start);
        printf("    push rbp\n");
        printf("    mov rbp, rsp\n");
        printf("    sub rsp, 256\n");

        reset_locals();
        in_function = 1;

        for (i = 0; i < node->data.func_decl.param_count; ++i)
        {
            thrive_ast *p_name = node->data.func_decl.params[i];
            thrive_var *v = add_var(p_name->data.name.start, p_name->data.name.length);
            printf("    mov [rbp%d], %s\n", v->offset, arg_regs[i]);
        }

        gen_stmt(node->data.func_decl.body);

        printf("    leave\n");
        printf("    ret\n");

        in_function = 0;
        break;
    }
    case THRIVE_AST_RETURN:
    {
        gen_expr(node->data.ret.expr);

        if (in_function)
        {
            printf("    leave\n");
            printf("    ret\n");
        }
        else
        {
            printf("    mov rcx, rax\n");
            printf("    call ExitProcess\n");
        }
        break;
    }

    case THRIVE_AST_EXT_DECL:
    {
        /* Do nothing here; handled in the top-level pass */
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

    /* Pass 1: Find and print all external declarations at the top */
    for (i = 0; i < node->data.block.count; ++i)
    {
        thrive_ast *stmt = node->data.block.items[i];
        if (stmt->kind == THRIVE_AST_EXT_DECL)
        {
            printf("extern %.*s\n",
                   stmt->data.ext_decl.name->data.name.length,
                   stmt->data.ext_decl.name->data.name.start);
        }
    }

    printf("\nsection .text\n");
    printf("global main\n\n");
    printf("main:\n");
    printf("    push rbp\n");
    printf("    mov rbp, rsp\n");
    printf("    sub rsp, 256\n");

    reset_locals();

    /* Pass 2: Main logic (Skip FUNC_DECL and EXT_DECL) */
    for (i = 0; i < node->data.block.count; ++i)
    {
        thrive_ast *stmt = node->data.block.items[i];
        if (stmt->kind != THRIVE_AST_FUNC_DECL && stmt->kind != THRIVE_AST_EXT_DECL)
        {
            gen_stmt(stmt);
        }
    }

    /* Pass 3: Internal Function Definitions */
    for (i = 0; i < node->data.block.count; ++i)
    {
        if (node->data.block.items[i]->kind == THRIVE_AST_FUNC_DECL)
        {
            gen_stmt(node->data.block.items[i]);
        }
    }

    /* Emit Data Section for Strings */
    if (string_count > 0)
    {
        printf("\nsection .data\n");
        for (i = 0; i < string_count; ++i)
        {
            u32 j;
            printf(".STR%d: db ", i);
            for (j = 0; j < string_pool[i].length; ++j)
            {
                if (string_pool[i].start[j] == '\\' && j + 1 < string_pool[i].length)
                {
                    j++;
                    switch (string_pool[i].start[j])
                    {
                    case 'n':
                        printf("10, ");
                        break;
                    case 'r':
                        printf("13, ");
                        break;
                    case 't':
                        printf("9, ");
                        break;
                    case '0':
                        printf("0, ");
                        break;
                    default:
                        printf("%d, ", string_pool[i].start[j]);
                        break;
                    }
                }
                else
                {
                    printf("%d, ", string_pool[i].start[j]);
                }
            }
            printf("0\n"); /* Null terminator */
        }
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

    case THRIVE_AST_FUNC_DECL:
    {
        u32 i;
        printf("FUNC_DECL %.*s\n",
               node->data.func_decl.name->data.name.length,
               node->data.func_decl.name->data.name.start);

        if (node->data.func_decl.param_count > 0)
        {
            thrive_print_indent(depth + 1);
            printf("PARAMS:\n");
            for (i = 0; i < node->data.func_decl.param_count; ++i)
            {
                thrive_ast_print(node->data.func_decl.params[i], depth + 2);
            }
        }

        thrive_print_indent(depth + 1);
        printf("BODY:\n");
        thrive_ast_print(node->data.func_decl.body, depth + 2);
        break;
    }

    case THRIVE_AST_FUNC_CALL:
    {
        u32 i;
        printf("FUNC_CALL %.*s\n",
               node->data.func_call.name->data.name.length,
               node->data.func_call.name->data.name.start);

        if (node->data.func_call.arg_count > 0)
        {
            thrive_print_indent(depth + 1);
            printf("ARGS:\n");
            for (i = 0; i < node->data.func_call.arg_count; ++i)
            {
                thrive_ast_print(node->data.func_call.args[i], depth + 2);
            }
        }
        break;
    }

    case THRIVE_AST_EXT_DECL:
    {
        printf("EXT_DECL %.*s\n",
               node->data.ext_decl.name->data.name.length,
               node->data.ext_decl.name->data.name.start);
        break;
    }

    default:
        printf("UNKNOWN AST\n");
        break;
    }
}

/* #############################################################################
 * # [SECTION] Assembler
 * #############################################################################
 */
static u8 binary_buffer[1024 * 64];
static u32 binary_pos = 0;

void emit_byte(u8 b)
{
    binary_buffer[binary_pos++] = b;
}

void emit_u32(u32 d)
{
    *(u32 *)&binary_buffer[binary_pos] = d;
    binary_pos += 4;
}

/* mov rax, <imm32>  -> 48 C7 C0 [xx xx xx xx] */
void emit_mov_rax_imm(u32 val)
{
    emit_byte(0x48);
    emit_byte(0xC7);
    emit_byte(0xC0);
    emit_u32(val);
}

void emit_push_rax()
{
    emit_byte(0x50);
}

void emit_pop_rbx()
{
    emit_byte(0x5B);
}

/* push rbp          -> 55 */
void emit_push_rbp()
{
    emit_byte(0x55);
}

/* pop rax           -> 58 */
void emit_pop_rax()
{
    emit_byte(0x58);
}

/* leave (mov rsp, rbp; pop rbp) -> C9 */
void emit_leave()
{
    emit_byte(0xC9);
}

/* ret               -> C3 */
void emit_ret()
{
    emit_byte(0xC3);
}

/* mov rbp, rsp      -> 48 89 E5 */
void emit_mov_rbp_rsp()
{
    emit_byte(0x48);
    emit_byte(0x89);
    emit_byte(0xE5);
}

/* sub rsp, imm32    -> 48 81 EC [xx xx xx xx] */
void emit_sub_rsp_imm(u32 val)
{
    emit_byte(0x48);
    emit_byte(0x81);
    emit_byte(0xEC);
    emit_u32(val);
}

/* mov [rbp + offset], rax -> 48 89 85 [offset32] */
void emit_mov_mrbp_rax(i32 offset)
{
    emit_byte(0x48);
    emit_byte(0x89);
    emit_byte(0x85);
    emit_u32((u32)offset);
}

/* mov rax, [rbp + offset] -> 48 8B 85 [offset32] */
void emit_mov_rax_mrbp(i32 offset)
{
    emit_byte(0x48);
    emit_byte(0x8B);
    emit_byte(0x85);
    emit_u32((u32)offset);
}

/* add rax, rbx      -> 48 01 D8 */
void emit_add_rax_rbx()
{
    emit_byte(0x48);
    emit_byte(0x01);
    emit_byte(0xD8);
}

/* sub rbx, rax      -> 48 29 C3 */
void emit_sub_rbx_rax()
{
    emit_byte(0x48);
    emit_byte(0x29);
    emit_byte(0xC3);
}

/* imul rax, rbx     -> 48 0F AF C3 */
void emit_imul_rax_rbx()
{
    emit_byte(0x48);
    emit_byte(0x0F);
    emit_byte(0xAF);
    emit_byte(0xC3);
}

/* xor rdx, rdx      -> 48 31 D2 */
void emit_xor_rdx_rdx()
{
    emit_byte(0x48);
    emit_byte(0x31);
    emit_byte(0xD2);
}

/* idiv rcx          -> 48 F7 F9 */
void emit_idiv_rcx()
{
    emit_byte(0x48);
    emit_byte(0xF7);
    emit_byte(0xF9);
}

/* cmp rbx, rax      -> 48 39 C3 */
void emit_cmp_rbx_rax()
{
    emit_byte(0x48);
    emit_byte(0x39);
    emit_byte(0xC3);
}

/* setl al           -> 0F 9C C0 */
void emit_setl_al()
{
    emit_byte(0x0F);
    emit_byte(0x9C);
    emit_byte(0xC0);
}

/* movzx rax, al     -> 48 0F B6 C0 */
void emit_movzx_rax_al()
{
    emit_byte(0x48);
    emit_byte(0x0F);
    emit_byte(0xB6);
    emit_byte(0xC0);
}

/* #############################################################################
 * # [SECTION] Testing
 * #############################################################################
 */
THRIVE_API void thrive_panic(thrive_status status)
{
    printf("[error] %s\n", status.message);
    printf(" --> input:%u:%u\n", status.line, status.column);
    printf("    |\n");
    printf("%3d | ", status.line);

    s8 *p = status.line_start;

    while (*p && *p != '\n')
    {
        putchar(*p);
        p++;
    }
    putchar('\n');
    printf("    | ");

    u32 offset = (u32)(status.token_start - status.line_start);

    u32 i;

    for (i = 0; i < offset; ++i)
    {
        putchar(' ');
    }

    u32 len = (u32)(status.token_end - status.token_start);

    if (len == 0)
    {
        len = 1;
    }

    putchar('^');

    for (i = 1; i < len; ++i)
    {
        putchar('~');
    }

    printf(" %s", status.message);

    putchar('\n');

    exit(1);
}

int main(void)
{
    s8 *source_code =
        "ext u32 MessageBoxA(u32 hWnd : s8 *lpText : s8 *lpCaption : u32 uType)\n"
        "ext u32 ExitProcess(u32 uExitCode)\n"
        "\n"
        "s8 *text = \"Hello World from THRIVE!\\nIt works!\"\n"
        "s8 *caption = \"Win32 Success\"\n"
        "\n"
        "u32 i = 0\n"
        "for (i = 0 : i < (1 + 2) : ++i)\n"
        "  MessageBoxA(0 : text : caption : 0)\n"
        "ExitProcess(0)\n";

    /*
    s8 *source_code =
        "ext u32 ExitProcess(u32 uExitCode)\n"
        "u32 i = (1 + 2) * 2 - 2\n"
        "ExitProcess(i)\n";

    s8 *source_code =
        "ext u32 ExitProcess(u32 uExitCode)\n"
        "u32 mul(u32 a : u32 b) {\n"
        " a * b\n"
        "}\n"
        "\n"
        "u32 add(u32 a : u32 b) {\n"
        " a + b\n"
        "}\n"
        "\n"
        "u32 result = mul(add(1 : 3) : 2)\n"
        "ExitProcess(result)\n";

    s8 *source_code =
        "u32  i = 1\n"
        "for(i = 0 : i < 10 : ++i) {\n"
        " continue\n"
        " i += 1\n"
        "}\n"
        "ret i\n";

    s8 *source_code =
        "u32  i = 1\n"
        "u32 *b = &i\n"
        "u32  c = *&i\n"
        "ret c\n";

    s8 *source_code =
        "u32  i = 1\n"
        "u32 *b = &i\n"
        "u32  c = *&i\n"
        "\n"
        "for(i = 0 : i < 10 : i++) {\n"
        " i += 1\n"
        "}\n"
        "\n"
        "ret i\n";

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
    state.line_start = source_code;
    state.source_code_size = thrive_string_length(source_code);
    state.ast_pool = malloc(sizeof(thrive_ast) * 1024);
    state.ast_capacity = 1024;

    printf("--------------------\n");
    printf("%s", source_code);
    printf("--------------------\n");
    printf("[size] thrive_status    = %10d\n", (u32)sizeof(thrive_status));
    printf("[size] thrive_token     = %10d\n", (u32)sizeof(thrive_token));
    printf("[size] thrive_state     = %10d\n", (u32)sizeof(thrive_state));
    printf("[size] thrive_ast_block = %10d\n", (u32)sizeof(thrive_ast_block));
    printf("[size] thrive_ast       = %10d\n", (u32)sizeof(thrive_ast));
    printf("--------------------\n");

    thrive_token_next(&state);

    print_token(state.current);

    while (state.current.kind)
    {
        thrive_token_next(&state);

        print_token(state.current);
    }

    free(state.ast_pool);

    /* Parse */
    printf("--------------------\n");
    {
        thrive_state s = {0};

        thrive_ast *ast;

        s.line = 1;
        s.column = 1;
        s.source_code = source_code;
        s.line_start = source_code;
        s.source_code_size = thrive_string_length(source_code);
        s.ast_pool = malloc(sizeof(thrive_ast) * 1024);
        s.ast_capacity = 1024;

        ast = thrive_ast_parse(&s);

        /*
        printf("=== BEFORE ===\n");
        thrive_ast_print(ast, 0);
        */

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
