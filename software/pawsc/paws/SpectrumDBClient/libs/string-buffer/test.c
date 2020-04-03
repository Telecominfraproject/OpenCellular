/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include "string_buffer.h"
#include <stdlib.h>
#include <stdio.h>


int main()
{
    {
        StrBuf *sb = strbuf_alloc();
        for (int i = 0; i < 100; i++)
            strbuf_append(sb, "a", 1);
        char *c_str = strbuf_to_cstr(sb);
        free(c_str);
        strbuf_free(sb);
    }

    {
        StrBuf *sb = strbuf_alloc();
        for (int i = 0; i < 100; i++)
            strbuf_append(sb, "abcdefghijklmnopqrstuvwxyz", rand() % 27);
        char *c_str = strbuf_to_cstr(sb);
        free(c_str);
        strbuf_free(sb);
    }

    return 0;
}