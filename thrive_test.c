
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

    return 0;
}
