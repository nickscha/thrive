
#include "thrive.h"
#include "stdio.h"
#include "stdlib.h"

/* #############################################################################
 * # [SECTION] Codegen x64 machine code
 * #############################################################################
 */
#define THRIVE_MAX_VARS 256
#define THRIVE_MAX_LABELS 1024
#define THRIVE_MAX_FIXUPS 1024
#define THRIVE_MAX_FUNCS 256

typedef struct
{
    s8 *start;
    u32 length;
    i32 offset;
    u8 is_array;
} thrive_var;

static thrive_var vars[THRIVE_MAX_VARS];
static u32 var_count = 0;
static i32 stack_offset = 0;
static i32 in_function = 0;

static i32 label_id = 0;
static i32 current_break_label = -1;
static i32 current_continue_label = -1;

/* Fixup System */
typedef enum
{
    FIXUP_JMP,
    FIXUP_CALL_REL,
    FIXUP_CALL_IAT,
    FIXUP_STRING
} fixup_type;

typedef struct
{
    fixup_type type;
    u32 buffer_offset;
    u32 instr_end_offset;
    i32 target_id;
} thrive_fixup;

static thrive_fixup fixups[THRIVE_MAX_FIXUPS];
static u32 fixup_count = 0;
static u32 label_offsets[THRIVE_MAX_LABELS];

typedef struct
{
    s8 *start;
    u32 length;
    u32 offset; /* Buffer offset where string is stored */
} thrive_string_data;
static thrive_string_data string_pool[256];
static u32 string_count = 0;

typedef struct
{
    s8 *start;
    u32 length;
    u32 rva;
    u8 is_external;
    u32 ext_dll_index;
    u32 ext_func_index;
} thrive_func;
static thrive_func funcs[THRIVE_MAX_FUNCS];
static u32 func_count = 0;

void reset_locals(void)
{
    var_count = 0;
    stack_offset = 0;
}
i32 new_label(void) { return label_id++; }

void bind_label(thrive_buffer *b, i32 label)
{
    label_offsets[label] = b->size;
}

void record_fixup(thrive_buffer *b, fixup_type type, i32 target_id)
{
    if (fixup_count < THRIVE_MAX_FIXUPS)
    {
        fixups[fixup_count].type = type;
        fixups[fixup_count].buffer_offset = b->size;
        fixups[fixup_count].instr_end_offset = b->size + 4;
        fixups[fixup_count].target_id = target_id;
        fixup_count++;
    }
    thrive_buffer_write_u32(b, 0); /* Dummy bytes to patch later */
}

i32 find_or_add_func(s8 *start, u32 length)
{
    u32 i;
    for (i = 0; i < func_count; ++i)
    {
        if (funcs[i].length == length && thrive_string_equals(funcs[i].start, start, length))
            return i;
    }
    funcs[func_count].start = start;
    funcs[func_count].length = length;
    return func_count++;
}

thrive_var *find_var(s8 *start, u32 length)
{
    u32 i;
    for (i = 0; i < var_count; ++i)
    {
        if (vars[i].length == length && thrive_string_equals(vars[i].start, start, length))
            return &vars[i];
    }
    return 0;
}

thrive_var *add_var(s8 *start, u32 length, u8 is_array, u32 array_size)
{
    thrive_var *v = &vars[var_count++];
    u32 size = is_array ? array_size : 1;
    stack_offset -= 8 * size;
    v->start = start;
    v->length = length;
    v->offset = stack_offset;
    v->is_array = is_array;
    return v;
}

void gen_expr(thrive_buffer *b, thrive_ast *node);
void gen_stmt(thrive_buffer *b, thrive_ast *node);

void gen_expr(thrive_buffer *b, thrive_ast *node)
{
    switch (node->kind)
    {
    case THRIVE_AST_INT:
        thrive_x64_mov_ri64(b, REG_RAX, node->data.int_value);
        break;
    case THRIVE_AST_NAME:
    {
        thrive_var *v = find_var(node->data.name.start, node->data.name.length);
        if (v->is_array)
        {
            thrive_x64_lea_r_mrbp(b, REG_RAX, v->offset);
        }
        else
        {
            thrive_x64_mov_r_mrbp(b, REG_RAX, v->offset);
        }
        break;
    }
    case THRIVE_AST_ARRAY_ACCESS:
        gen_expr(b, node->data.array_access.index);
        thrive_x64_mov_ri32(b, REG_RBX, 8);
        thrive_x64_imul_rr(b, REG_RAX, REG_RBX); /* rax = index * 8 */
        thrive_x64_push_r(b, REG_RAX);

        gen_expr(b, node->data.array_access.left);
        thrive_x64_pop_r(b, REG_RBX);
        thrive_x64_add_rr(b, REG_RAX, REG_RBX);   /* rax = base + offset */
        thrive_x64_mov_r_mr(b, REG_RAX, REG_RAX); /* rax = [rax] */
        break;
    case THRIVE_AST_BINARY:
    {
        gen_expr(b, node->data.binary.left);
        thrive_x64_push_r(b, REG_RAX);
        gen_expr(b, node->data.binary.right);
        thrive_x64_pop_r(b, REG_RBX); /* RBX=left, RAX=right */

        switch (node->data.binary.op)
        {
        case THRIVE_TOKEN_KIND_ADD:
            thrive_x64_add_rr(b, REG_RAX, REG_RBX);
            break;
        case THRIVE_TOKEN_KIND_SUB:
            thrive_x64_sub_rr(b, REG_RBX, REG_RAX);
            thrive_x64_mov_rr(b, REG_RAX, REG_RBX);
            break;
        case THRIVE_TOKEN_KIND_MUL:
            thrive_x64_imul_rr(b, REG_RAX, REG_RBX);
            break;
        case THRIVE_TOKEN_KIND_DIV:
            thrive_x64_xor_rr(b, REG_RDX, REG_RDX);
            thrive_x64_mov_rr(b, REG_RCX, REG_RAX); /* RCX = right */
            thrive_x64_mov_rr(b, REG_RAX, REG_RBX); /* RAX = left */
            thrive_x64_idiv_r(b, REG_RCX);
            break;
        case THRIVE_TOKEN_KIND_LT:
            thrive_x64_cmp_rr(b, REG_RBX, REG_RAX);
            thrive_x64_setcc(b, CC_L);
            thrive_x64_movzx_rax_al(b);
            break;
        case THRIVE_TOKEN_KIND_GT:
            thrive_x64_cmp_rr(b, REG_RBX, REG_RAX);
            thrive_x64_setcc(b, CC_G);
            thrive_x64_movzx_rax_al(b);
            break;
        case THRIVE_TOKEN_KIND_LT_EQUALS:
            thrive_x64_cmp_rr(b, REG_RBX, REG_RAX);
            thrive_x64_setcc(b, CC_LE);
            thrive_x64_movzx_rax_al(b);
            break;
        case THRIVE_TOKEN_KIND_GT_EQUALS:
            thrive_x64_cmp_rr(b, REG_RBX, REG_RAX);
            thrive_x64_setcc(b, CC_GE);
            thrive_x64_movzx_rax_al(b);
            break;
        case THRIVE_TOKEN_KIND_EQUALS:
            thrive_x64_cmp_rr(b, REG_RBX, REG_RAX);
            thrive_x64_setcc(b, CC_E);
            thrive_x64_movzx_rax_al(b);
            break;
        case THRIVE_TOKEN_KIND_NOT_EQUALS:
            thrive_x64_cmp_rr(b, REG_RBX, REG_RAX);
            thrive_x64_setcc(b, CC_NE);
            thrive_x64_movzx_rax_al(b);
            break;
        case THRIVE_TOKEN_KIND_AND_BITWISE:
            thrive_x64_and_rr(b, REG_RAX, REG_RBX);
            break;
        case THRIVE_TOKEN_KIND_OR_BITWISE:
            thrive_x64_or_rr(b, REG_RAX, REG_RBX);
            break;
        case THRIVE_TOKEN_KIND_AND_LOGICAL:
        {
            i32 l_false = new_label(), l_end = new_label();
            gen_expr(b, node->data.binary.left);
            thrive_x64_test_rr(b, REG_RAX, REG_RAX);
            thrive_buffer_write_u8(b, 0x0F);
            thrive_buffer_write_u8(b, 0x80 | CC_E);
            record_fixup(b, FIXUP_JMP, l_false);
            gen_expr(b, node->data.binary.right);
            thrive_x64_test_rr(b, REG_RAX, REG_RAX);
            thrive_buffer_write_u8(b, 0x0F);
            thrive_buffer_write_u8(b, 0x80 | CC_E);
            record_fixup(b, FIXUP_JMP, l_false);
            thrive_x64_mov_ri64(b, REG_RAX, 1);
            thrive_buffer_write_u8(b, 0xE9);
            record_fixup(b, FIXUP_JMP, l_end);
            bind_label(b, l_false);
            thrive_x64_mov_ri64(b, REG_RAX, 0);
            bind_label(b, l_end);
            break;
        }
        case THRIVE_TOKEN_KIND_OR_LOGICAL:
        {
            i32 l_true = new_label(), l_end = new_label();
            gen_expr(b, node->data.binary.left);
            thrive_x64_test_rr(b, REG_RAX, REG_RAX);
            thrive_buffer_write_u8(b, 0x0F);
            thrive_buffer_write_u8(b, 0x80 | CC_NE);
            record_fixup(b, FIXUP_JMP, l_true);
            gen_expr(b, node->data.binary.right);
            thrive_x64_test_rr(b, REG_RAX, REG_RAX);
            thrive_buffer_write_u8(b, 0x0F);
            thrive_buffer_write_u8(b, 0x80 | CC_NE);
            record_fixup(b, FIXUP_JMP, l_true);
            thrive_x64_mov_ri64(b, REG_RAX, 0);
            thrive_buffer_write_u8(b, 0xE9);
            record_fixup(b, FIXUP_JMP, l_end);
            bind_label(b, l_true);
            thrive_x64_mov_ri64(b, REG_RAX, 1);
            bind_label(b, l_end);
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
            thrive_var *v = find_var(node->data.unary.expr->data.name.start, node->data.unary.expr->data.name.length);
            thrive_x64_mov_r_mrbp(b, REG_RAX, v->offset);
            if (node->data.unary.op == THRIVE_TOKEN_KIND_INC)
            {
                thrive_x64_mov_ri32(b, REG_RBX, 1);
                thrive_x64_add_rr(b, REG_RAX, REG_RBX);
            }
            else
            {
                thrive_x64_mov_ri32(b, REG_RBX, 1);
                thrive_x64_sub_rr(b, REG_RAX, REG_RBX);
            }
            thrive_x64_mov_mrbp_r(b, v->offset, REG_RAX);
        }
        else
        {
            gen_expr(b, node->data.unary.expr);
            switch (node->data.unary.op)
            {
            case THRIVE_TOKEN_KIND_SUB:
                thrive_x64_neg_r(b, REG_RAX);
                break;
            case THRIVE_TOKEN_KIND_NEGATE:
                thrive_x64_test_rr(b, REG_RAX, REG_RAX);
                thrive_x64_setcc(b, CC_E);
                thrive_x64_movzx_rax_al(b);
                break;
            default:
                break;
            }
        }
        break;
    }
    case THRIVE_AST_TERNARY:
    {
        i32 l_else = new_label(), l_end = new_label();
        gen_expr(b, node->data.ternary.cond);
        thrive_x64_test_rr(b, REG_RAX, REG_RAX);
        thrive_buffer_write_u8(b, 0x0F);
        thrive_buffer_write_u8(b, 0x80 | CC_E);
        record_fixup(b, FIXUP_JMP, l_else);
        gen_expr(b, node->data.ternary.then_expr);
        thrive_buffer_write_u8(b, 0xE9);
        record_fixup(b, FIXUP_JMP, l_end);
        bind_label(b, l_else);
        gen_expr(b, node->data.ternary.else_expr);
        bind_label(b, l_end);
        break;
    }
    case THRIVE_AST_ASSIGN:
    {
        thrive_ast *left = node->data.assign.left;
        thrive_ast *right = node->data.assign.right;

        if (left->kind == THRIVE_AST_DEREF)
        {
            gen_expr(b, right);
            thrive_x64_push_r(b, REG_RAX);
            gen_expr(b, left->data.unary.expr);
            thrive_x64_pop_r(b, REG_RBX);
            thrive_x64_mov_mr_r(b, REG_RAX, REG_RBX);
        }
        else if (left->kind == THRIVE_AST_ARRAY_ACCESS)
        {
            gen_expr(b, right);
            thrive_x64_push_r(b, REG_RAX);
            gen_expr(b, left->data.array_access.index);
            thrive_x64_mov_ri32(b, REG_RBX, 8);
            thrive_x64_imul_rr(b, REG_RAX, REG_RBX);
            thrive_x64_push_r(b, REG_RAX);
            gen_expr(b, left->data.array_access.left);
            thrive_x64_pop_r(b, REG_RBX);
            thrive_x64_add_rr(b, REG_RAX, REG_RBX);
            thrive_x64_pop_r(b, REG_RBX);
            thrive_x64_mov_mr_r(b, REG_RAX, REG_RBX);
        }
        else
        {
            gen_expr(b, right);
            thrive_var *v = find_var(left->data.name.start, left->data.name.length);
            thrive_x64_mov_mrbp_r(b, v->offset, REG_RAX);
        }
        break;
    }
    case THRIVE_AST_ADDR_OF:
    {
        thrive_var *v = find_var(node->data.unary.expr->data.name.start, node->data.unary.expr->data.name.length);
        thrive_x64_lea_r_mrbp(b, REG_RAX, v->offset);
        break;
    }
    case THRIVE_AST_DEREF:
        gen_expr(b, node->data.unary.expr);
        thrive_x64_mov_r_mr(b, REG_RAX, REG_RAX);
        break;
    case THRIVE_AST_BREAK:
        thrive_buffer_write_u8(b, 0xE9);
        record_fixup(b, FIXUP_JMP, current_break_label);
        break;
    case THRIVE_AST_CONTINUE:
        thrive_buffer_write_u8(b, 0xE9);
        record_fixup(b, FIXUP_JMP, current_continue_label);
        break;
    case THRIVE_AST_FUNC_CALL:
    {
        thrive_ast *curr = node->data.func_call.args;
        thrive_x64_reg arg_regs[] = {REG_RCX, REG_RDX, REG_R8, REG_R9};
        u32 arg_count = 0, i, stack_cleanup;
        i32 f_idx = find_or_add_func(node->data.func_call.name->data.name.start, node->data.func_call.name->data.name.length);

        while (curr)
        {
            gen_expr(b, curr);
            thrive_x64_push_r(b, REG_RAX);
            arg_count++;
            curr = curr->next;
        }

        u32 reg_args = arg_count > 4 ? 4 : arg_count;
        for (i = reg_args; i > 0; --i)
        {
            thrive_x64_pop_r(b, arg_regs[i - 1]);
        }

        thrive_x64_sub_rsp_imm32(b, 32); /* Shadow space */

        if (funcs[f_idx].is_external)
        {
            thrive_buffer_write_u8(b, 0xFF);
            thrive_buffer_write_u8(b, 0x15);
            record_fixup(b, FIXUP_CALL_IAT, f_idx);
        }
        else
        {
            thrive_buffer_write_u8(b, 0xE8);
            record_fixup(b, FIXUP_CALL_REL, f_idx);
        }

        stack_cleanup = 32 + (arg_count > 4 ? (arg_count - 4) * 8 : 0);
        thrive_x64_add_rsp_imm32(b, stack_cleanup);
        break;
    }
    case THRIVE_AST_STRING:
    {
        u32 id = string_count++;
        string_pool[id].start = node->data.string_lit.start;
        string_pool[id].length = node->data.string_lit.length;

        thrive_x64_rex(b, 1, REG_RAX, 0);
        thrive_buffer_write_u8(b, 0x8D); /* LEA RAX, [rel STR] */
        thrive_buffer_write_u8(b, 0x05);
        record_fixup(b, FIXUP_STRING, id);
        break;
    }
    case THRIVE_TOKEN_KIND_LSHIFT:
        thrive_x64_mov_rr(b, REG_RCX, REG_RAX);
        thrive_x64_shl_cl(b, REG_RBX);
        thrive_x64_mov_rr(b, REG_RAX, REG_RBX);
        break;
    case THRIVE_TOKEN_KIND_RSHIFT:
        thrive_x64_mov_rr(b, REG_RCX, REG_RAX);
        thrive_x64_shr_cl(b, REG_RBX);
        thrive_x64_mov_rr(b, REG_RAX, REG_RBX);
        break;
    default:
        break;
    }
}

void gen_stmt(thrive_buffer *b, thrive_ast *node)
{
    switch (node->kind)
    {
    case THRIVE_AST_DECL:
    {
        thrive_ast *name = node->data.decl.name;
        thrive_var *v = add_var(name->data.name.start, name->data.name.length, node->data.decl.is_array, node->data.decl.array_size);
        if (node->data.decl.value)
        {
            gen_expr(b, node->data.decl.value);
            thrive_x64_mov_mrbp_r(b, v->offset, REG_RAX);
        }
        break;
    }
    case THRIVE_AST_IF:
    {
        i32 l_else = new_label(), l_end = new_label();
        gen_expr(b, node->data.if_stmt.cond);
        thrive_x64_test_rr(b, REG_RAX, REG_RAX);

        thrive_buffer_write_u8(b, 0x0F);
        thrive_buffer_write_u8(b, 0x80 | CC_E);
        record_fixup(b, FIXUP_JMP, node->data.if_stmt.else_branch ? l_else : l_end);

        gen_stmt(b, node->data.if_stmt.then_branch);

        if (node->data.if_stmt.else_branch)
        {
            thrive_buffer_write_u8(b, 0xE9);
            record_fixup(b, FIXUP_JMP, l_end);
            bind_label(b, l_else);
            gen_stmt(b, node->data.if_stmt.else_branch);
        }
        bind_label(b, l_end);
        break;
    }
    case THRIVE_AST_FOR:
    {
        i32 start_label = new_label(), step_label = new_label(), end_label = new_label();
        i32 old_break = current_break_label, old_continue = current_continue_label;
        current_break_label = end_label;
        current_continue_label = step_label;

        gen_expr(b, node->data.for_loop.init);
        bind_label(b, start_label);
        gen_expr(b, node->data.for_loop.cond);
        thrive_x64_test_rr(b, REG_RAX, REG_RAX);
        thrive_buffer_write_u8(b, 0x0F);
        thrive_buffer_write_u8(b, 0x80 | CC_E);
        record_fixup(b, FIXUP_JMP, end_label);

        gen_stmt(b, node->data.for_loop.body);

        bind_label(b, step_label);
        gen_expr(b, node->data.for_loop.step);
        thrive_buffer_write_u8(b, 0xE9);
        record_fixup(b, FIXUP_JMP, start_label);
        bind_label(b, end_label);

        current_break_label = old_break;
        current_continue_label = old_continue;
        break;
    }
    case THRIVE_AST_BLOCK:
    {
        thrive_ast *curr = node->data.block.body;
        while (curr)
        {
            gen_stmt(b, curr);
            curr = curr->next;
        }
        break;
    }
    case THRIVE_AST_FUNC_DECL:
    {
        thrive_ast *curr = node->data.func_decl.params;
        thrive_x64_reg arg_regs[] = {REG_RCX, REG_RDX, REG_R8, REG_R9};
        u32 p_idx = 0;

        i32 f_idx = find_or_add_func(node->data.func_decl.name->data.name.start, node->data.func_decl.name->data.name.length);
        funcs[f_idx].rva = 0x1000 + b->size;

        thrive_x64_push_r(b, REG_RBP);
        thrive_x64_mov_rr(b, REG_RBP, REG_RSP);
        thrive_x64_sub_rsp_imm32(b, 256);

        u32 saved_var_count = var_count;
        i32 saved_stack_offset = stack_offset;

        stack_offset = 0;

        in_function = 1;
        while (curr && p_idx < 4)
        {
            thrive_var *v = add_var(curr->data.name.start, curr->data.name.length, 0, 0);
            thrive_x64_mov_mrbp_r(b, v->offset, arg_regs[p_idx++]);
            curr = curr->next;
        }

        gen_stmt(b, node->data.func_decl.body);

        thrive_x64_leave(b);
        thrive_x64_ret(b);

        var_count = saved_var_count;
        stack_offset = saved_stack_offset;
        in_function = 0;
        break;
    }
    case THRIVE_AST_RETURN:
        gen_expr(b, node->data.ret.expr);
        if (in_function)
        {
            thrive_x64_leave(b);
            thrive_x64_ret(b);
        }
        else
        {
            /* If we are at the top-level scope, default behaviour: ExitProcess */
            thrive_x64_mov_rr(b, REG_RCX, REG_RAX);
            i32 exit_idx = find_or_add_func((s8 *)"ExitProcess", 11);
            funcs[exit_idx].is_external = 1; /* Best effort fallback */
            thrive_x64_sub_rsp_imm32(b, 32);
            thrive_buffer_write_u8(b, 0xFF);
            thrive_buffer_write_u8(b, 0x15);
            record_fixup(b, FIXUP_CALL_IAT, exit_idx);
            thrive_x64_add_rsp_imm32(b, 32);
        }
        break;
    default:
        gen_expr(b, node);
        break;
    }
}

/* Hardcoded mapping for demonstration */
static s8 *kUser32 = "user32.dll";
static s8 *kKernel32 = "kernel32.dll";
static s8 *user32_funcs[32];
static s8 *kernel32_funcs[32];
static u32 u32_fc = 0, k32_fc = 0;
static s8 import_name_pool[1024];
static u32 import_name_pool_offset = 0;

void gen_program(thrive_buffer *code_b, thrive_ast *node, thrive_buffer *exe_out)
{
    thrive_ast *curr;
    u32 i;
    thrive_p32_plus_import imports[2];
    u32 num_imports = 0;

    func_count = 0;
    fixup_count = 0;
    string_count = 0;

    /* Pass 1: Collect External Decl */
    import_name_pool_offset = 0; /* Reset pool for fresh generations */

    curr = node->data.block.body;
    while (curr)
    {
        if (curr->kind == THRIVE_AST_EXT_DECL)
        {
            i32 f_idx = find_or_add_func(curr->data.ext_decl.name->data.name.start, curr->data.ext_decl.name->data.name.length);
            funcs[f_idx].is_external = 1;

            /* Extract and NULL-terminate the function name for the PE Importer */
            u32 len = funcs[f_idx].length;
            s8 *null_terminated_name = &import_name_pool[import_name_pool_offset];
            u32 j;

            for (j = 0; j < len; ++j)
            {
                null_terminated_name[j] = funcs[f_idx].start[j];
            }
            null_terminated_name[len] = '\0';
            import_name_pool_offset += len + 1;

            /* Use the null-terminated version for the imports table */
            if (null_terminated_name[0] == 'M' && null_terminated_name[1] == 'e' && null_terminated_name[2] == 's')
            { /* MessageBoxA */
                funcs[f_idx].ext_dll_index = 0;
                funcs[f_idx].ext_func_index = u32_fc;
                user32_funcs[u32_fc++] = (char *)null_terminated_name;
            }
            else
            {
                funcs[f_idx].ext_dll_index = 1;
                funcs[f_idx].ext_func_index = k32_fc;
                kernel32_funcs[k32_fc++] = (char *)null_terminated_name;
            }
        }
        curr = curr->next;
    }

    if (u32_fc > 0)
    {
        imports[num_imports].library = kUser32;
        imports[num_imports].imports = user32_funcs;
        imports[num_imports].imports_count = u32_fc;
        /* Re-map dll indices */
        for (i = 0; i < func_count; ++i)
        {
            if (funcs[i].is_external && funcs[i].ext_dll_index == 0)
                funcs[i].ext_dll_index = num_imports;
        }
        num_imports++;
    }
    if (k32_fc > 0)
    {
        imports[num_imports].library = kKernel32;
        imports[num_imports].imports = kernel32_funcs;
        imports[num_imports].imports_count = k32_fc;
        for (i = 0; i < func_count; ++i)
        {
            if (funcs[i].is_external && funcs[i].ext_dll_index == 1)
                funcs[i].ext_dll_index = num_imports;
        }
        num_imports++;
    }

    /* Pass 2: Main Logic */
    reset_locals();
    thrive_x64_push_r(code_b, REG_RBP);
    thrive_x64_mov_rr(code_b, REG_RBP, REG_RSP);
    thrive_x64_sub_rsp_imm32(code_b, 256);

    curr = node->data.block.body;
    while (curr)
    {
        if (curr->kind != THRIVE_AST_FUNC_DECL && curr->kind != THRIVE_AST_EXT_DECL)
            gen_stmt(code_b, curr);
        curr = curr->next;
    }
    thrive_x64_leave(code_b);
    thrive_x64_ret(code_b);

    /* Pass 3: Internal Functions */
    curr = node->data.block.body;
    while (curr)
    {
        if (curr->kind == THRIVE_AST_FUNC_DECL)
            gen_stmt(code_b, curr);
        curr = curr->next;
    }

    /* Pass 4: Embed Strings into Text Section */
    for (i = 0; i < string_count; ++i)
    {
        u32 j;
        string_pool[i].offset = code_b->size;
        for (j = 0; j < string_pool[i].length; ++j)
        {
            if (string_pool[i].start[j] == '\\' && j + 1 < string_pool[i].length)
            {
                j++;
                switch (string_pool[i].start[j])
                {
                case 'n':
                    thrive_buffer_write_u8(code_b, 10);
                    break;
                case 'r':
                    thrive_buffer_write_u8(code_b, 13);
                    break;
                case 't':
                    thrive_buffer_write_u8(code_b, 9);
                    break;
                case '0':
                    thrive_buffer_write_u8(code_b, 0);
                    break;
                default:
                    thrive_buffer_write_u8(code_b, string_pool[i].start[j]);
                    break;
                }
            }
            else
            {
                thrive_buffer_write_u8(code_b, string_pool[i].start[j]);
            }
        }
        thrive_buffer_write_u8(code_b, 0); /* Null Terminator */
    }

    /* Recalculate IAT RVAs dynamically based on final text size */
    for (i = 0; i < func_count; ++i)
    {
        if (funcs[i].is_external)
        {
            funcs[i].rva = thrive_pe32_plus_get_iat_rva(imports, num_imports, code_b->size, funcs[i].ext_dll_index, funcs[i].ext_func_index);
        }
    }

    /* Pass 5: Apply Fixups */
    u32 text_rva = 0x1000;
    for (i = 0; i < fixup_count; ++i)
    {
        thrive_fixup *f = &fixups[i];
        i32 rel = 0;

        if (f->type == FIXUP_JMP)
        {
            rel = (i32)label_offsets[f->target_id] - (i32)f->instr_end_offset;
        }
        else if (f->type == FIXUP_CALL_REL)
        {
            u32 internal_target_offset = funcs[f->target_id].rva - text_rva;
            rel = (i32)internal_target_offset - (i32)f->instr_end_offset;
        }
        else if (f->type == FIXUP_CALL_IAT)
        {
            u32 target_rva = funcs[f->target_id].rva;
            u32 curr_rva = text_rva + f->instr_end_offset;
            rel = (i32)target_rva - (i32)curr_rva;
        }
        else if (f->type == FIXUP_STRING)
        {
            rel = (i32)string_pool[f->target_id].offset - (i32)f->instr_end_offset;
        }

        u32 *patch_ptr = (u32 *)(code_b->data + f->buffer_offset);
        *patch_ptr = (u32)rel;
    }

    /* Finally, emit executable via your PE32+ generator */
    thrive_pe32_plus_generate(exe_out, code_b, imports, num_imports, code_b->size);
}

#include "thrive_ast_print.h"

/* #############################################################################
 * # [SECTION] Testing
 * #############################################################################
 */
THRIVE_API void thrive_panic(thrive_status status)
{

    s8 *p = status.line_start;
    u32 i;
    u32 offset;
    u32 len;

    printf("[error] %s\n", status.message);
    printf(" --> input:%u:%u\n", status.line, status.column);
    printf("    |\n");
    printf("%3d | ", status.line);

    while (*p && *p != '\n')
    {
        putchar(*p);
        p++;
    }
    putchar('\n');
    printf("    | ");

    offset = (u32)(status.token_start - status.line_start);

    for (i = 0; i < offset; ++i)
    {
        putchar(' ');
    }

    len = (u32)(status.token_end - status.token_start);

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
        "; this is a line comment\n"
        "ext u32 Sleep(u32 milliseconds)\n"
        "ext u32 MessageBoxA(u32 hWnd : s8 *lpText : s8 *lpCaption : u32 uType)\n"
        "ext u32 ExitProcess(u32 uExitCode)\n"
        "\n"
        "s8 *text = \"Hello World from THRIVE!\\nIt works!\"\n"
        "s8 *text2 = \"arr[1] == 10\"\n"
        "s8 *caption = \"Win32 Success\"\n"
        "u32 arr[3]\n"
        "arr[0] = 5\n"
        "arr[1] = arr[0] + 5\n"
        "arr[2] = 15\n"
        "u32 *ptr = arr\n"
        "\n"
        "u32 add(u32 a : u32 b) {\n"
        " ret a + b\n"
        "}\n"
        "\n"
        "u32 i = 0\n"
        "for (i = 0 : i < (0x01 + 0b10) : ++i) {\n"
        "  MessageBoxA(0 : text : caption : 0)\n"
        "  if (i == 0) {\n"
        "     continue\n"
        "  } else if (i == add(0 : 1) && ptr[i] == 10) {\n"
        "     Sleep(1000)\n"
        "     MessageBoxA(0 : text2 : caption : 0)\n"
        "     break\n"
        "  }\n"
        "}\n"
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
            u8 x64_data[8192];
            thrive_buffer code_buffer = {0};
            code_buffer.data = x64_data;
            code_buffer.capacity = 8192;

            u8 pe_data[16384];
            thrive_buffer exe_buffer = {0};
            exe_buffer.data = pe_data;
            exe_buffer.capacity = 16384;

            gen_program(&code_buffer, ast, &exe_buffer);

            /* Output Executable */
            FILE *f = fopen("test.exe", "wb");
            if (f)
            {
                fwrite(exe_buffer.data, 1, exe_buffer.size, f);
                fclose(f);
                printf("[thrive] generated: test.exe (%u bytes).\n", exe_buffer.size);
            }
        }

        printf("\n");
        printf("---------------------------------------------\n");
        printf("ast_count       : %12d\n", s.ast_count);
        printf("ast_size (bytes): %12d\n", s.ast_count * sizeof(thrive_ast));
        printf("ast_size (kb)   : %12.6f\n", (f64)(s.ast_count * sizeof(thrive_ast)) / 1024.0);
        printf("ast_size (mb)   : %12.6f\n", (f64)(s.ast_count * sizeof(thrive_ast)) / 1024.0 / 1024.0);
        printf("---------------------------------------------\n");
    }

    return 0;
}
