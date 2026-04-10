
#include "thrive.h"
#include "stdio.h"
#include "stdlib.h"

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
        "s8 *text3 = \"Goodbye\"\n"
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
        "MessageBoxA(0 : text3 : caption : 0)\n"
        "ExitProcess(0)\n";

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

            thrive_x64_codegen_program(&code_buffer, ast, &exe_buffer);

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
