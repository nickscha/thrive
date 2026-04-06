#include "thrive.h"
#include "stdio.h"
#include "stdlib.h"

THRIVE_API void thrive_panic(thrive_status status)
{
    (void)status;
}

/* #############################################################################
 * # [SECTION] 64 bit types
 * #############################################################################
 */
#if __STDC_VERSION__ >= 199901L
typedef long long i64;
typedef unsigned long long u64;
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlong-long"
typedef long long i64;
typedef unsigned long long u64;
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
typedef __int64 i64;
typedef unsigned __int64 u64;
#else
typedef long i64;
typedef unsigned long u64;
#endif

/* #############################################################################
 * # [SECTION] PE Builder
 * #############################################################################
 */
typedef struct buf_t
{
    u8 *data;
    u32 size;
    u32 capacity;
} buf_t;

void emit_u8(buf_t *b, u8 v) { b->data[b->size++] = v; }

void emit_u16(buf_t *b, u16 v)
{
    emit_u8(b, v & 0xFF);
    emit_u8(b, (v >> 8) & 0xFF);
}

void emit_u32(buf_t *b, u32 v)
{
    emit_u8(b, v & 0xFF);
    emit_u8(b, (v >> 8) & 0xFF);
    emit_u8(b, (v >> 16) & 0xFF);
    emit_u8(b, (v >> 24) & 0xFF);
}

void emit_u64(buf_t *b, u64 v)
{
    emit_u8(b, (u8)(v & 0xFF));
    emit_u8(b, (u8)((v >> 8) & 0xFF));
    emit_u8(b, (u8)((v >> 16) & 0xFF));
    emit_u8(b, (u8)((v >> 24) & 0xFF));
    emit_u8(b, (u8)((v >> 32) & 0xFF));
    emit_u8(b, (u8)((v >> 40) & 0xFF));
    emit_u8(b, (u8)((v >> 48) & 0xFF));
    emit_u8(b, (u8)((v >> 56) & 0xFF));
}

void pad_to(buf_t *b, u32 align)
{
    while (b->size % align)
    {
        emit_u8(b, 0);
    }
}

/* #############################################################################
 * # [SECTION] x64 Instruction Emitters
 * #############################################################################
 */
typedef enum
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

void emit_modrm_reg(buf_t *b, x64_reg reg, x64_reg rm)
{
    /* Mod = 11 (register-direct addressing) */
    u8 modrm = 0xC0 | ((reg & 7) << 3) | (rm & 7);
    emit_u8(b, modrm);
}

void emit_rex(buf_t *b, u8 w, x64_reg reg, x64_reg rm)
{
    u8 rex = 0x40;
    if (w)
        rex |= 0x08; /* W: 64-bit operand size */
    if (reg >= 8)
        rex |= 0x04; /* R: Extension for 'reg' field */
    if (rm >= 8)
        rex |= 0x01; /* B: Extension for 'rm' field */
    emit_u8(b, rex);
}

#define OP_EXT_ADD 0
#define OP_EXT_SUB 5
#define OP_EXT_MOV 0

/* SUB reg, imm8 or ADD reg, imm8 */
void emit_alu_ri8(buf_t *b, u8 op_ext, x64_reg dst, u8 imm)
{
    emit_rex(b, 1, 0, dst);
    emit_u8(b, 0x83);
    emit_u8(b, 0xC0 | (op_ext << 3) | (dst & 7));
    emit_u8(b, imm);
}

/* MOV reg, imm32 */
void emit_mov_ri32(buf_t *b, x64_reg dst, u32 imm)
{
    emit_rex(b, 1, 0, dst);
    emit_u8(b, 0xC7);
    emit_u8(b, 0xC0 | (OP_EXT_MOV << 3) | (dst & 7));
    emit_u32(b, imm);
}

/* MOV reg, imm64 */
void emit_mov_ri64(buf_t *b, x64_reg dst, u64 imm)
{
    emit_rex(b, 1, 0, dst);
    emit_u8(b, 0xB8 | (dst & 7));
    emit_u64(b, imm);
}

/* PUSH reg */
void emit_push_r(buf_t *b, x64_reg reg)
{
    if (reg >= 8)
    {
        emit_u8(b, 0x41); /* REX.B for high registers */
    }
    emit_u8(b, 0x50 | (reg & 7));
}

/* XOR reg, reg */
void emit_xor_rr(buf_t *b, x64_reg dst, x64_reg src)
{
    emit_rex(b, 1, src, dst);
    emit_u8(b, 0x31);
    emit_modrm_reg(b, src, dst);
}

/* MOV reg, reg */
void emit_mov_rr(buf_t *b, x64_reg dst, x64_reg src)
{
    emit_rex(b, 1, src, dst);
    emit_u8(b, 0x8B);
    emit_modrm_reg(b, src, dst);
}

void emit_inst_call_rel32(buf_t *b, u32 curr_rva, u32 target_rva)
{
    u32 rel = target_rva - (curr_rva + 6);
    emit_u8(b, 0xFF); /* CALL */
    emit_u8(b, 0x15); /* ModR/M: rel32 */
    emit_u32(b, rel);
}

/* #############################################################################
 * # [SECTION] Import Directory
 * #############################################################################
 */
typedef struct thrive_import
{
    s8 *library;       /* e.g. "kernel32.dll" */
    s8 **imports;      /* e.g. "ExitProcess", "Sleep" */
    u32 imports_count; /* count of imports entries */
    u32 *iat_rvas;     /* Output: populates with resolved IAT RVAs */
} thrive_import;

/* Standard limits to avoid heap allocations in C89 */
#define MAX_DLLS 16
#define MAX_TOTAL_IMPORTS 512

u32 emit_idata(buf_t *b, u32 rva_base, thrive_import *imports)
{
    u32 start = b->size;
    u32 i, j, dll_count = 0;
    u32 idt_offset;
    u32 pool_idx = 0;

    u32 int_rvas[MAX_DLLS];
    u32 iat_rvas[MAX_DLLS];
    u32 dll_name_rvas[MAX_DLLS];

    u32 func_rva_pool[MAX_TOTAL_IMPORTS];
    u32 *func_name_rvas[MAX_DLLS];

    while (imports[dll_count].library != NULL && dll_count < MAX_DLLS)
    {
        func_name_rvas[dll_count] = &func_rva_pool[pool_idx];
        pool_idx += imports[dll_count].imports_count;

        if (pool_idx >= MAX_TOTAL_IMPORTS)
        {
            break; /* TODO: Safety check */
        }
        dll_count++;
    }

    if (dll_count == 0)
        return 0;

    /* 2. IDT (Import Directory Table) + Null Terminator Descriptor */
    idt_offset = b->size;
    for (i = 0; i < (dll_count + 1) * 20; ++i)
    {
        emit_u8(b, 0);
    }

    /* 3. INT (Import Name Table) */
    for (i = 0; i < dll_count; ++i)
    {
        int_rvas[i] = rva_base + (b->size - start);
        for (j = 0; j < imports[i].imports_count + 1; ++j)
        {
            emit_u64(b, 0); /* 64-bit RVAs */
        }
    }

    /* 4. IAT (Import Address Table) */
    for (i = 0; i < dll_count; ++i)
    {
        iat_rvas[i] = rva_base + (b->size - start);

        for (j = 0; j < imports[i].imports_count; ++j)
        {
            if (imports[i].iat_rvas != NULL)
            {
                imports[i].iat_rvas[j] = iat_rvas[i] + (j * 8);
            }
        }

        for (j = 0; j < imports[i].imports_count + 1; ++j)
        {
            emit_u64(b, 0);
        }
    }

    /* 5. Function Names & Hints */
    for (i = 0; i < dll_count; ++i)
    {
        for (j = 0; j < imports[i].imports_count; ++j)
        {
            s8 *p = imports[i].imports[j];
            func_name_rvas[i][j] = rva_base + (b->size - start);

            emit_u16(b, 0); /* Hint */
            while (*p)
            {
                emit_u8(b, *p++);
            }
            emit_u8(b, 0);
        }
    }

    /* 6. DLL Names */
    for (i = 0; i < dll_count; ++i)
    {
        s8 *p = imports[i].library;
        dll_name_rvas[i] = rva_base + (b->size - start);
        while (*p)
        {
            emit_u8(b, *p++);
        }
        emit_u8(b, 0);
    }

    /* 7. Fill INT and IAT slots with the strings' RVAs */
    for (i = 0; i < dll_count; ++i)
    {
        u32 int_off = (int_rvas[i] - rva_base) + start;
        u32 iat_off = (iat_rvas[i] - rva_base) + start;

        for (j = 0; j < imports[i].imports_count; ++j)
        {
            *(u64 *)(b->data + int_off + (j * 8)) = (u64)func_name_rvas[i][j];
            *(u64 *)(b->data + iat_off + (j * 8)) = (u64)func_name_rvas[i][j];
        }
    }

    /* 8. Link everything back into the IDT */
    for (i = 0; i < dll_count; ++i)
    {
        u32 entry_off = idt_offset + (i * 20);
        *(u32 *)(b->data + entry_off + 0) = int_rvas[i];       /* INT RVA */
        *(u32 *)(b->data + entry_off + 4) = 0;                 /* TimeDateStamp */
        *(u32 *)(b->data + entry_off + 8) = 0;                 /* ForwarderChain */
        *(u32 *)(b->data + entry_off + 12) = dll_name_rvas[i]; /* Name RVA */
        *(u32 *)(b->data + entry_off + 16) = iat_rvas[i];      /* IAT RVA */
    }

    return b->size - start;
}

void emit_x86_64_pe32_plus(thrive_import *imports, buf_t *out, u8 *code, u32 code_size)
{
    u32 pe_offset = 0x80;
    u32 image_base = 0x400000;
    u32 section_align = 0x1000;
    u32 file_align = 0x200;
    u32 size_of_headers = 0x200;

    /* Dynamic Size Calculation */
    u32 text_rva = 0x1000;
    u32 text_raw = 0x200;
    u32 text_raw_size = (code_size + 0x1FF) & ~0x1FF;
    u32 text_virt_size = (code_size + 0xFFF) & ~0xFFF;

    u32 idata_rva = text_rva + text_virt_size;
    u32 idata_raw = text_raw + text_raw_size;

    /* Dummy run to find out exactly how big .idata will be */
    buf_t dummy = {0};
    u32 actual_idata_size = 0;
    u32 idata_raw_size = 0;
    u32 idata_virt_size = 0;
    u32 size_of_image = 0;
    i32 i;

    dummy.data = (u8 *)malloc(8192); /* Generous dummy buffer */
    actual_idata_size = emit_idata(&dummy, idata_rva, imports);
    free(dummy.data);

    idata_raw_size = (actual_idata_size + 0x1FF) & ~0x1FF;
    idata_virt_size = (actual_idata_size + 0xFFF) & ~0xFFF;
    size_of_image = idata_rva + idata_virt_size;

    /* DOS Header */
    emit_u16(out, 0x5A4D); /* MZ */

    while (out->size < 0x3C)
    {
        emit_u8(out, 0);
    }

    emit_u32(out, pe_offset);

    while (out->size < pe_offset)
    {
        emit_u8(out, 0);
    }

    /* PE Signature & COFF Header */
    emit_u32(out, 0x00004550);
    emit_u16(out, 0x8664);
    emit_u16(out, 2);
    emit_u32(out, 0);
    emit_u32(out, 0);
    emit_u32(out, 0);
    emit_u16(out, 0xF0);
    emit_u16(out, 0x0022);

    /* Optional Headers (PE32+) */
    emit_u16(out, 0x20B);
    emit_u8(out, 0);
    emit_u8(out, 0);
    emit_u32(out, code_size);
    emit_u32(out, 0);
    emit_u32(out, 0);
    emit_u32(out, text_rva);
    emit_u32(out, text_rva);
    emit_u64(out, image_base);
    emit_u32(out, section_align);
    emit_u32(out, file_align);
    emit_u16(out, 4);
    emit_u16(out, 0);
    emit_u16(out, 0);
    emit_u16(out, 0);
    emit_u16(out, 4);
    emit_u16(out, 0);
    emit_u32(out, 0);
    emit_u32(out, size_of_image); /* Updated to be dynamic */
    emit_u32(out, size_of_headers);
    emit_u32(out, 0);
    emit_u16(out, 3);
    emit_u16(out, 0);
    emit_u64(out, 0x100000);
    emit_u64(out, 0x1000);
    emit_u64(out, 0x100000);
    emit_u64(out, 0x1000);
    emit_u32(out, 0);
    emit_u32(out, 16);

    for (i = 0; i < 16; ++i)
    {
        if (i == 1)
        {
            emit_u32(out, idata_rva);
            emit_u32(out, actual_idata_size); /* Using precise size */
        }
        else
        {
            emit_u32(out, 0);
            emit_u32(out, 0);
        }
    }

    /* .text Section Header */
    emit_u8(out, '.');
    emit_u8(out, 't');
    emit_u8(out, 'e');
    emit_u8(out, 'x');
    emit_u8(out, 't');
    emit_u8(out, 0);
    emit_u8(out, 0);
    emit_u8(out, 0);
    emit_u32(out, code_size);
    emit_u32(out, text_rva);
    emit_u32(out, text_raw_size);
    emit_u32(out, text_raw);
    emit_u32(out, 0);
    emit_u32(out, 0);
    emit_u16(out, 0);
    emit_u16(out, 0);
    emit_u32(out, 0x60000020); /* Code | Exec | Read */

    /* .idata Section Header */
    emit_u8(out, '.');
    emit_u8(out, 'i');
    emit_u8(out, 'd');
    emit_u8(out, 'a');
    emit_u8(out, 't');
    emit_u8(out, 'a');
    emit_u8(out, 0);
    emit_u8(out, 0);
    emit_u32(out, idata_virt_size);
    emit_u32(out, idata_rva);
    emit_u32(out, idata_raw_size);
    emit_u32(out, idata_raw);
    emit_u32(out, 0);
    emit_u32(out, 0);
    emit_u16(out, 0);
    emit_u16(out, 0);
    emit_u32(out, 0x40000040); /* Read */

    pad_to(out, file_align);

    /* .TEXT Section Body */
    while (out->size < text_raw)
    {
        emit_u8(out, 0);
    }

    for (i = 0; i < (i32)code_size; ++i)
    {
        emit_u8(out, code[i]);
    }
    pad_to(out, file_align);

    /* .IDATA Section Body */
    while (out->size < idata_raw)
    {
        emit_u8(out, 0);
    }
    emit_idata(out, idata_rva, imports); /* Write the actual data */
    pad_to(out, file_align);
}

int main(void)
{
    /* Define Imports */
    s8 *k32_funcs[] = {"ExitProcess", "Sleep"};
    u32 k32_iats[2] = {0};

    /* Example of a second library */
    s8 *u32_funcs[] = {"MessageBoxA"};
    u32 u32_iats[1] = {0};

    thrive_import imports[] = {
        {"kernel32.dll", k32_funcs, 2, k32_iats},
        {"user32.dll", u32_funcs, 1, u32_iats},
        {NULL, NULL, 0, NULL} /* Null terminator */
    };

    /* 1. Dry Run to Resolve IAT RVAs */
    buf_t tmp = {0};
    tmp.data = (u8 *)malloc(8192);
    u32 estimated_text_virt_size = 0x1000;
    u32 idata_rva_base = 0x1000 + estimated_text_virt_size;

    emit_idata(&tmp, idata_rva_base, imports);
    free(tmp.data);

    /* 2. Machine Code Generation */
    u8 code[256];
    buf_t codebuf = {0};
    codebuf.data = code;

    /* Sleep(2000) */
    emit_alu_ri8(&codebuf, OP_EXT_SUB, REG_RSP, 32); /* sub rsp, 32 */
    emit_mov_ri32(&codebuf, REG_RCX, 2000);          /* mov rcx, 2000 */
    emit_inst_call_rel32(&codebuf, 0x1000 + codebuf.size, k32_iats[1]);
    emit_alu_ri8(&codebuf, OP_EXT_ADD, REG_RSP, 32); /* add rsp, 32 */

    /* ExitProcess(0) */
    emit_alu_ri8(&codebuf, OP_EXT_SUB, REG_RSP, 32); /* sub rsp, 32 */
    emit_xor_rr(&codebuf, REG_RCX, REG_RCX);
    emit_inst_call_rel32(&codebuf, 0x1000 + codebuf.size, k32_iats[0]); /* ExitProcess IAT */
    emit_alu_ri8(&codebuf, OP_EXT_ADD, REG_RSP, 32);                    /* add rsp, 32 */

    /* 3. Build PE */
    buf_t buf = {0};
    buf.data = (u8 *)malloc(8192 * sizeof(u8));
    buf.capacity = 8192;

    emit_x86_64_pe32_plus(imports, &buf, codebuf.data, codebuf.size);

    FILE *f = fopen("out.exe", "wb");

    if (f)
    {
        fwrite(buf.data, 1, buf.size, f);
        fclose(f);
    }

    free(buf.data);
    printf("finished\n");

    return 0;
}
