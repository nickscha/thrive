#include "thrive.h"
#include "stdio.h"
#include "stdlib.h"

THRIVE_API void thrive_panic(thrive_status status)
{
    (void)status;
}

/* #############################################################################
 * # [SECTION] x64 Instruction Emitters
 * #############################################################################
 */
typedef enum x64_reg
{
    REG_RAX = 0,
    REG_RCX = 1,
    REG_RDX = 2,
    REG_RBX = 3,
    REG_RSP = 4,
    REG_RBP = 5,
    REG_RSI = 6,
    REG_RDI = 7,
    REG_R8 = 8,
    REG_R9 = 9,
    REG_R10 = 10,
    REG_R11 = 11,
    REG_R12 = 12,
    REG_R13 = 13,
    REG_R14 = 14,
    REG_R15 = 15
} x64_reg;

typedef enum x64_op_ext
{
    OP_EXT_ADD = 0,
    OP_EXT_SUB = 5,
    OP_EXT_MOV = 0
} x64_op_ext;

typedef enum x64_cc
{
    CC_E = 4,
    CC_NE = 5,
    CC_L = 12,
    CC_LE = 14,
    CC_G = 15,
    CC_GE = 13
} x64_cc;

void emit_modrm_reg(thrive_buffer *b, x64_reg reg, x64_reg rm)
{
    /* Mod = 11 (register-direct addressing) */
    u8 modrm = 0xC0 | ((reg & 7) << 3) | (rm & 7);
    thrive_buffer_write_u8(b, modrm);
}

void emit_rex(thrive_buffer *b, u8 w, x64_reg reg, x64_reg rm)
{
    u8 rex = 0x40;

    if (w)
    {
        rex |= 0x08; /* W: 64-bit operand size */
    }

    if (reg >= 8)
    {
        rex |= 0x04; /* R: Extension for 'reg' field */
    }

    if (rm >= 8)
    {
        rex |= 0x01; /* B: Extension for 'rm' field */
    }

    thrive_buffer_write_u8(b, rex);
}

/* SUB reg, imm8 or ADD reg, imm8 */
void emit_alu_ri8(thrive_buffer *b, x64_op_ext op_ext, x64_reg dst, u8 imm)
{
    emit_rex(b, 1, 0, dst);
    thrive_buffer_write_u8(b, 0x83);
    thrive_buffer_write_u8(b, 0xC0 | (op_ext << 3) | (dst & 7));
    thrive_buffer_write_u8(b, imm);
}

/* [rbp + disp32] */
void emit_modrm_disp32(thrive_buffer *b, x64_reg reg, x64_reg base, i32 disp)
{
    thrive_buffer_write_u8(b, 0x80 | ((reg & 7) << 3) | (base & 7)); /* mod=10 */
    thrive_buffer_write_u32(b, disp);
}

/* MOV reg, imm32 */
void emit_mov_ri32(thrive_buffer *b, x64_reg dst, u32 imm)
{
    emit_rex(b, 1, 0, dst);
    thrive_buffer_write_u8(b, 0xC7);
    thrive_buffer_write_u8(b, 0xC0 | (OP_EXT_MOV << 3) | (dst & 7));
    thrive_buffer_write_u32(b, imm);
}

/* MOV reg, imm64 */
void emit_mov_ri64(thrive_buffer *b, x64_reg dst, u64 imm)
{
    emit_rex(b, 1, 0, dst);
    thrive_buffer_write_u8(b, 0xB8 | (dst & 7));
    thrive_buffer_write_u64(b, imm);
}

/* mov r64, [rbp+disp] */
void emit_mov_r_mrbp(thrive_buffer *b, x64_reg dst, i32 disp)
{
    emit_rex(b, 1, dst, REG_RBP);
    thrive_buffer_write_u8(b, 0x8B);
    emit_modrm_disp32(b, dst, REG_RBP, disp);
}

/* mov [rbp+disp], r64 */
void emit_mov_mrbp_r(thrive_buffer *b, i32 disp, x64_reg src)
{
    emit_rex(b, 1, src, REG_RBP);
    thrive_buffer_write_u8(b, 0x89);
    emit_modrm_disp32(b, src, REG_RBP, disp);
}

/* lea r64, [rbp+disp] */
void emit_lea_r_mrbp(thrive_buffer *b, x64_reg dst, i32 disp)
{
    emit_rex(b, 1, dst, REG_RBP);
    thrive_buffer_write_u8(b, 0x8D);
    emit_modrm_disp32(b, dst, REG_RBP, disp);
}

/* MOV reg, reg */
void emit_mov_rr(thrive_buffer *b, x64_reg dst, x64_reg src)
{
    emit_rex(b, 1, dst, src);
    thrive_buffer_write_u8(b, 0x8B);
    emit_modrm_reg(b, dst, src);
}

/* mov rax, [rax] */
void emit_mov_r_mr(thrive_buffer *b, x64_reg dst, x64_reg base)
{
    emit_rex(b, 1, dst, base);
    thrive_buffer_write_u8(b, 0x8B);
    emit_modrm_reg(b, dst, base);
}

/* mov [rax], rbx */
void emit_mov_mr_r(thrive_buffer *b, x64_reg base, x64_reg src)
{
    emit_rex(b, 1, src, base);
    thrive_buffer_write_u8(b, 0x89);
    emit_modrm_reg(b, src, base);
}

/* movzx rax, al */
void emit_movzx_rax_al(thrive_buffer *b)
{
    thrive_buffer_write_u8(b, 0x48);
    thrive_buffer_write_u8(b, 0x0F);
    thrive_buffer_write_u8(b, 0xB6);
    thrive_buffer_write_u8(b, 0xC0);
}

/* PUSH reg */
void emit_push_r(thrive_buffer *b, x64_reg reg)
{
    if (reg >= 8)
    {
        thrive_buffer_write_u8(b, 0x41); /* REX.B for high registers */
    }
    thrive_buffer_write_u8(b, 0x50 | (reg & 7));
}

/* POP reg */
void emit_pop_r(thrive_buffer *b, x64_reg reg)
{
    if (reg >= 8)
    {
        thrive_buffer_write_u8(b, 0x41);
    }
    thrive_buffer_write_u8(b, 0x58 | (reg & 7));
}

/* XOR reg, reg */
void emit_xor_rr(thrive_buffer *b, x64_reg dst, x64_reg src)
{
    emit_rex(b, 1, src, dst);
    thrive_buffer_write_u8(b, 0x31);
    emit_modrm_reg(b, src, dst);
}

/* add r64, r64 */
void emit_add_rr(thrive_buffer *b, x64_reg dst, x64_reg src)
{
    emit_rex(b, 1, src, dst);
    thrive_buffer_write_u8(b, 0x01);
    emit_modrm_reg(b, src, dst);
}

/* sub r64, r64 */
void emit_sub_rr(thrive_buffer *b, x64_reg dst, x64_reg src)
{
    emit_rex(b, 1, src, dst);
    thrive_buffer_write_u8(b, 0x29);
    emit_modrm_reg(b, src, dst);
}

/* imul r64, r64 */
void emit_imul_rr(thrive_buffer *b, x64_reg dst, x64_reg src)
{
    emit_rex(b, 1, dst, src);
    thrive_buffer_write_u8(b, 0x0F);
    thrive_buffer_write_u8(b, 0xAF);
    emit_modrm_reg(b, dst, src);
}

/* cmp r64, r64 */
void emit_cmp_rr(thrive_buffer *b, x64_reg a, x64_reg rb)
{
    emit_rex(b, 1, rb, a);
    thrive_buffer_write_u8(b, 0x39);
    emit_modrm_reg(b, rb, a);
}

/* setcc */
void emit_setcc(thrive_buffer *b, x64_cc cc)
{
    thrive_buffer_write_u8(b, 0x0F);
    thrive_buffer_write_u8(b, 0x90 | cc); /* cc = condition */
    thrive_buffer_write_u8(b, 0xC0);      /* AL */
}

/* jmp rel32 */
void emit_jmp(thrive_buffer *b, i32 rel)
{
    thrive_buffer_write_u8(b, 0xE9);
    thrive_buffer_write_u32(b, rel);
}

/* je / jne / etc */
void emit_jcc(thrive_buffer *b, x64_cc cc, i32 rel)
{
    thrive_buffer_write_u8(b, 0x0F);
    thrive_buffer_write_u8(b, 0x80 | cc);
    thrive_buffer_write_u32(b, rel);
}

/* direct call */
void emit_call_rel32(thrive_buffer *b, u32 curr_rva, u32 target_rva)
{
    u32 rel = target_rva - (curr_rva + 5);
    thrive_buffer_write_u8(b, 0xE8);
    thrive_buffer_write_u32(b, rel);
}

void emit_inst_call_rel32(thrive_buffer *b, u32 curr_rva, u32 target_rva)
{
    u32 rel = target_rva - (curr_rva + 6);
    thrive_buffer_write_u8(b, 0xFF); /* CALL */
    thrive_buffer_write_u8(b, 0x15); /* ModR/M: rel32 */
    thrive_buffer_write_u32(b, rel);
}

/* #############################################################################
 * # [SECTION] Import Directory
 * #############################################################################
 */
i32 main(void)
{
    /* 1. Define Imports */
    s8 *k32_funcs[] = {"Sleep", "ExitProcess"};
    s8 *u32_funcs[] = {"MessageBoxA"};

    thrive_library_import imports[] = {
        {"kernel32.dll", k32_funcs, 2},
        {"user32.dll", u32_funcs, 1},
    };

    u32 imports_count = sizeof(imports) / sizeof(imports[0]);

    u32 text_vsize = 0x1000;

    /* 2. Pre-calculate IAT RVAs dynamically */
    u32 sleep_rva = thrive_pe32_plus_get_iat_rva(imports, imports_count, text_vsize, 0, 0);
    u32 exit_rva = thrive_pe32_plus_get_iat_rva(imports, imports_count, text_vsize, 0, 1);
    u32 messagebox_rva = thrive_pe32_plus_get_iat_rva(imports, imports_count, text_vsize, 1, 0);

    /* 3. Machine Code Generation */
    u8 code[256];
    thrive_buffer codebuf = {0};
    codebuf.data = code;

    {
        /* --- Call 1: Sleep(2000) --- */
        emit_alu_ri8(&codebuf, OP_EXT_SUB, REG_RSP, 40);
        emit_mov_ri32(&codebuf, REG_RCX, 2000);
        emit_inst_call_rel32(&codebuf, 0x1000 + codebuf.size, sleep_rva);
        emit_alu_ri8(&codebuf, OP_EXT_ADD, REG_RSP, 40);

        /* --- Call 2: MessageBoxA(0, "Thrive", "Thrive", 0) --- */
        emit_mov_ri64(&codebuf, REG_RAX, 0x0000657669726854ULL); /* "Thrive\0\0" LE */
        emit_push_r(&codebuf, REG_RAX);
        emit_mov_rr(&codebuf, REG_RDX, REG_RSP); /* lpText */
        emit_mov_rr(&codebuf, REG_R8, REG_RSP);  /* lpCaption */
        emit_xor_rr(&codebuf, REG_RCX, REG_RCX); /* hWnd = 0 */
        emit_xor_rr(&codebuf, REG_R9, REG_R9);   /* uType = MB_OK (0) */
        emit_alu_ri8(&codebuf, OP_EXT_SUB, REG_RSP, 32);
        emit_inst_call_rel32(&codebuf, 0x1000 + codebuf.size, messagebox_rva);
        emit_alu_ri8(&codebuf, OP_EXT_ADD, REG_RSP, 40);

        /* --- Call 3: ExitProcess(0) --- */
        emit_alu_ri8(&codebuf, OP_EXT_SUB, REG_RSP, 40);
        emit_xor_rr(&codebuf, REG_RCX, REG_RCX);
        emit_inst_call_rel32(&codebuf, 0x1000 + codebuf.size, exit_rva);
    }

    /* 4. Emit PE File */
    u8 pe_data[8192];
    thrive_buffer pe_buf = {0};
    pe_buf.data = pe_data;

    thrive_pe32_plus_generate(
        &pe_buf,
        &codebuf,
        imports,
        imports_count,
        text_vsize);

    /* Output */
    FILE *f = fopen("out.exe", "wb");
    if (f)
    {
        fwrite(pe_buf.data, 1, pe_buf.size, f);
        fclose(f);
        printf("Successfully generated out.exe (%u bytes).\n", pe_buf.size);
    }

    return 0;
}
