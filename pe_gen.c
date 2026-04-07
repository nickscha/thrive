#include "thrive.h"
#include "stdio.h"
#include "stdlib.h"

THRIVE_API void thrive_panic(thrive_status status)
{
    (void)status;
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

    thrive_p32_plus_import imports[] = {
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
        thrive_x64_alu_ri8(&codebuf, OP_EXT_SUB, REG_RSP, 40);
        thrive_x64_mov_ri32(&codebuf, REG_RCX, 2000);
        thrive_x64_inst_call_rel32(&codebuf, 0x1000 + codebuf.size, sleep_rva);
        thrive_x64_alu_ri8(&codebuf, OP_EXT_ADD, REG_RSP, 40);

        /* --- Call 2: MessageBoxA(0, "Thrive", "Thrive", 0) --- */
        thrive_x64_mov_ri64(&codebuf, REG_RAX, 0x0000657669726854ULL); /* "Thrive\0\0" LE */
        thrive_x64_push_r(&codebuf, REG_RAX);
        thrive_x64_mov_rr(&codebuf, REG_RDX, REG_RSP); /* lpText */
        thrive_x64_mov_rr(&codebuf, REG_R8, REG_RSP);  /* lpCaption */
        thrive_x64_xor_rr(&codebuf, REG_RCX, REG_RCX); /* hWnd = 0 */
        thrive_x64_xor_rr(&codebuf, REG_R9, REG_R9);   /* uType = MB_OK (0) */
        thrive_x64_alu_ri8(&codebuf, OP_EXT_SUB, REG_RSP, 32);
        thrive_x64_inst_call_rel32(&codebuf, 0x1000 + codebuf.size, messagebox_rva);
        thrive_x64_alu_ri8(&codebuf, OP_EXT_ADD, REG_RSP, 40);

        /* --- Call 3: ExitProcess(0) --- */
        thrive_x64_alu_ri8(&codebuf, OP_EXT_SUB, REG_RSP, 40);
        thrive_x64_xor_rr(&codebuf, REG_RCX, REG_RCX);
        thrive_x64_inst_call_rel32(&codebuf, 0x1000 + codebuf.size, exit_rva);
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
        printf("[thrive] generated: out.exe (%u bytes).\n", pe_buf.size);
    }

    return 0;
}
