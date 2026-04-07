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
 * # [SECTION] PE Utilities
 * #############################################################################
 */
typedef struct thrive_import
{
    s8 *library;       /* e.g. "kernel32.dll" */
    s8 **imports;      /* e.g. "ExitProcess", "Sleep" */
    u32 imports_count; /* count of imports entries */

} thrive_import;

static u32 align_up(u32 val, u32 align)
{
    if (align == 0)
    {
        return val;
    }
    return (val + align - 1) & ~(align - 1);
}

/* * Allows your emitter to fetch the exact IAT target RVA for a specific
 * function before generating the instruction, avoiding a 2-pass fixup.
 */
u32 get_iat_rva(thrive_import *imports, u32 num_imports, u32 text_vsize, u32 dll_index, u32 func_index)
{
    u32 text_rva = 0x1000;
    u32 rdata_rva = text_rva + align_up(text_vsize, 0x1000);
    u32 idt_size = (num_imports + 1) * 20;

    u32 num_funcs = 0;
    u32 i;

    for (i = 0; i < num_imports; ++i)
    {
        num_funcs += imports[i].imports_count;
    }

    u32 ilt_size = (num_funcs + num_imports) * 8;
    u32 iat_base_rva = rdata_rva + idt_size + ilt_size;

    u32 offset = 0;

    for (i = 0; i < dll_index; ++i)
    {
        offset += (imports[i].imports_count + 1) * 8;
    }

    offset += func_index * 8;

    return iat_base_rva + offset;
}

/* #############################################################################
 * # [SECTION] PE32+ Builder
 * #############################################################################
 */

void emit_pe32_file(
    thrive_buffer *out,
    thrive_buffer *code,
    thrive_import *imports,
    u32 num_imports,
    u32 text_vsize)
{
    u32 i, j;
    u32 num_funcs = 0;
    u32 strings_size = 0;

    /* Calculate Import String sizes */
    for (i = 0; i < num_imports; ++i)
    {
        num_funcs += imports[i].imports_count;
        strings_size += thrive_string_length(imports[i].library) + 1;
        for (j = 0; j < imports[i].imports_count; j++)
        {
            strings_size += 2 + thrive_string_length((s8 *)imports[i].imports[j]) + 1; /* 2 bytes for Hint */
        }
    }

    /* Section Layout Calculations */
    u32 idt_size = (num_imports + 1) * 20;
    u32 ilt_size = (num_funcs + num_imports) * 8;
    u32 iat_size = ilt_size;

    u32 text_rva = 0x1000;
    u32 text_raw_size = align_up(code->size, 0x200);

    u32 rdata_rva = text_rva + align_up(text_vsize, 0x1000);
    u32 rdata_vsize = idt_size + ilt_size + iat_size + strings_size;
    u32 rdata_raw_size = align_up(rdata_vsize, 0x200);

    u32 idt_rva = rdata_rva;
    u32 ilt_rva = idt_rva + idt_size;
    u32 iat_rva = ilt_rva + ilt_size;
    u32 strings_rva = iat_rva + iat_size;
    u32 size_of_image = align_up(rdata_rva + rdata_vsize, 0x1000);

    /* Write DOS Header */
    thrive_buffer_write_u16(out, 0x5A4D); /* MZ */
    thrive_buffer_align(out, 0x3C);       /* */
    thrive_buffer_write_u32(out, 0x40);   /* e_lfanew */

    /* Write NT Headers */
    thrive_buffer_write_u32(out, 0x00004550); /* PE\0\0 */
    thrive_buffer_write_u16(out, 0x8664);     /* Machine: AMD64 */
    thrive_buffer_write_u16(out, 2);          /* NumberOfSections */
    thrive_buffer_write_u32(out, 0);          /* TimeDateStamp */
    thrive_buffer_write_u32(out, 0);          /* PointerToSymbolTable */
    thrive_buffer_write_u32(out, 0);          /* NumberOfSymbols */
    thrive_buffer_write_u16(out, 0xF0);       /* SizeOfOptionalHeader */
    thrive_buffer_write_u16(out, 0x0022);     /* Characteristics (Exec, LargeAddr) */

    /* Write Optional Header (PE32+) */
    thrive_buffer_write_u16(out, 0x020B);                /* Magic */
    thrive_buffer_write_u8(out, 1);                      /* MajorLinkerVersion */
    thrive_buffer_write_u8(out, 0);                      /* MinorLinkerVersion */
    thrive_buffer_write_u32(out, text_raw_size);         /* SizeOfCode */
    thrive_buffer_write_u32(out, rdata_raw_size);        /* SizeOfInitializedData */
    thrive_buffer_write_u32(out, 0);                     /* SizeOfUninitializedData */
    thrive_buffer_write_u32(out, text_rva);              /* AddressOfEntryPoint */
    thrive_buffer_write_u32(out, text_rva);              /* BaseOfCode */
    thrive_buffer_write_u64(out, 0x0000000140000000ULL); /* ImageBase */
    thrive_buffer_write_u32(out, 0x1000);                /* SectionAlignment */
    thrive_buffer_write_u32(out, 0x200);                 /* FileAlignment */
    thrive_buffer_write_u16(out, 5);                     /* */
    thrive_buffer_write_u16(out, 2);                     /* OSMajor / OSMinor */
    thrive_buffer_write_u16(out, 0);                     /* */
    thrive_buffer_write_u16(out, 0);                     /* ImageMajor / ImageMinor */
    thrive_buffer_write_u16(out, 5);                     /* */
    thrive_buffer_write_u16(out, 2);                     /* SubSysMajor / SubSysMinor */
    thrive_buffer_write_u32(out, 0);                     /* Win32VersionValue */
    thrive_buffer_write_u32(out, size_of_image);         /* SizeOfImage */
    thrive_buffer_write_u32(out, 0x200);                 /* SizeOfHeaders */
    thrive_buffer_write_u32(out, 0);                     /* CheckSum */
    thrive_buffer_write_u16(out, 3);                     /* Subsystem (3 = Windows CUI Console) */
    thrive_buffer_write_u16(out, 0x8140);                /* DllCharacteristics (NX, DynBase, HighEnt) */
    thrive_buffer_write_u64(out, 0x100000);              /* SizeOfStackReserve */
    thrive_buffer_write_u64(out, 0x1000);                /* SizeOfStackCommit */
    thrive_buffer_write_u64(out, 0x100000);              /* SizeOfHeapReserve */
    thrive_buffer_write_u64(out, 0x1000);                /* SizeOfHeapCommit */
    thrive_buffer_write_u32(out, 0);                     /* LoaderFlags */
    thrive_buffer_write_u32(out, 16);                    /* NumberOfRvaAndSizes */

    /* Data Directories */
    thrive_buffer_write_u32(out, 0);
    thrive_buffer_write_u32(out, 0); /* Export Directory */
    thrive_buffer_write_u32(out, idt_rva);
    thrive_buffer_write_u32(out, idt_size); /* Import Directory */

    for (i = 2; i < 16; ++i)
    {
        thrive_buffer_write_u32(out, 0);
        thrive_buffer_write_u32(out, 0);
    }

    /* Section Header: .text */
    thrive_buffer_write_bytes(out, (u8 *)".text\0\0\0", 8);
    thrive_buffer_write_u32(out, code->size);    /* VirtualSize */
    thrive_buffer_write_u32(out, text_rva);      /* VirtualAddress */
    thrive_buffer_write_u32(out, text_raw_size); /* SizeOfRawData */
    thrive_buffer_write_u32(out, 0x200);         /* PointerToRawData */
    thrive_buffer_write_u32(out, 0);             /* */
    thrive_buffer_write_u32(out, 0);             /* */
    thrive_buffer_write_u16(out, 0);             /* */
    thrive_buffer_write_u16(out, 0);             /* */
    thrive_buffer_write_u32(out, 0x60000020);    /* Characteristics (RX) */

    /* Section Header: .rdata */
    thrive_buffer_write_bytes(out, (u8 *)".rdata\0\0", 8);
    thrive_buffer_write_u32(out, rdata_vsize);           /* VirtualSize */
    thrive_buffer_write_u32(out, rdata_rva);             /* VirtualAddress */
    thrive_buffer_write_u32(out, rdata_raw_size);        /* SizeOfRawData */
    thrive_buffer_write_u32(out, 0x200 + text_raw_size); /* PointerToRawData */
    thrive_buffer_write_u32(out, 0);                     /* */
    thrive_buffer_write_u32(out, 0);                     /* */
    thrive_buffer_write_u16(out, 0);                     /* */
    thrive_buffer_write_u16(out, 0);                     /* */
    thrive_buffer_write_u32(out, 0x40000040);            /* Characteristics (R) */

    thrive_buffer_align(out, 0x200); /* Pad Header to File Alignment */

    /* Write .text Data */
    thrive_buffer_write_bytes(out, code->data, code->size);
    thrive_buffer_align(out, 0x200);

    /* Write .rdata Data (Imports) */
    u32 current_dll_name_rva = strings_rva;
    u32 current_func_name_rva = strings_rva;

    for (i = 0; i < num_imports; ++i)
    {
        current_func_name_rva += thrive_string_length(imports[i].library) + 1;
    }

    u32 current_ilt_rva = ilt_rva;
    u32 current_iat_rva = iat_rva;

    /* Write Import Descriptor Table (IDT) */
    for (i = 0; i < num_imports; ++i)
    {
        thrive_buffer_write_u32(out, current_ilt_rva);
        thrive_buffer_write_u32(out, 0);
        thrive_buffer_write_u32(out, 0);
        thrive_buffer_write_u32(out, current_dll_name_rva);
        thrive_buffer_write_u32(out, current_iat_rva);

        current_dll_name_rva += thrive_string_length(imports[i].library) + 1;
        current_ilt_rva += (imports[i].imports_count + 1) * 8;
        current_iat_rva += (imports[i].imports_count + 1) * 8;
    }

    thrive_buffer_write_u32(out, 0);
    thrive_buffer_write_u32(out, 0);
    thrive_buffer_write_u32(out, 0);
    thrive_buffer_write_u32(out, 0);
    thrive_buffer_write_u32(out, 0);

    /* Write Import Lookup Table (ILT) & Import Address Table (IAT) */
    {
        i32 table;

        for (table = 0; table < 2; ++table)
        {
            u32 temp_name_rva = current_func_name_rva;

            for (i = 0; i < num_imports; ++i)
            {
                for (j = 0; j < imports[i].imports_count; ++j)
                {
                    thrive_buffer_write_u64(out, temp_name_rva);
                    temp_name_rva += 2 + thrive_string_length((s8 *)imports[i].imports[j]) + 1;
                }
                thrive_buffer_write_u64(out, 0); /* Null thunk bounds the array */
            }
        }

        /* Write Import Strings: DLL Names */
        for (i = 0; i < num_imports; ++i)
        {
            thrive_buffer_write_bytes(out, (u8 *)imports[i].library, thrive_string_length(imports[i].library) + 1);
        }

        /* Write Import Strings: Function Names */
        for (i = 0; i < num_imports; ++i)
        {
            for (j = 0; j < imports[i].imports_count; ++j)
            {
                thrive_buffer_write_u16(out, 0); /* Ordinal Hint */
                thrive_buffer_write_bytes(out, (u8 *)imports[i].imports[j], thrive_string_length((s8 *)imports[i].imports[j]) + 1);
            }
        }
    }

    thrive_buffer_align(out, 0x200); /* Final File Alignment Pad */
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

    thrive_import imports[] = {
        {"kernel32.dll", k32_funcs, 2},
        {"user32.dll", u32_funcs, 1},
    };

    u32 imports_count = sizeof(imports) / sizeof(imports[0]);

    u32 text_vsize = 0x1000;

    /* 2. Pre-calculate IAT RVAs dynamically */
    u32 sleep_rva = get_iat_rva(imports, imports_count, text_vsize, 0, 0);
    u32 exit_rva = get_iat_rva(imports, imports_count, text_vsize, 0, 1);
    u32 messagebox_rva = get_iat_rva(imports, imports_count, text_vsize, 1, 0);

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

    emit_pe32_file(
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
