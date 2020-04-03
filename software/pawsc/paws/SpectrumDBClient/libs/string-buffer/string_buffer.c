/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#include "string_buffer.h"

#include <stdlib.h>
#include <string.h>


StrBuf *strbuf_alloc()
{
    StrBuf *buf = malloc(sizeof(StrBuf));
    if (!buf) return NULL;

    int initial_size = 32;
    buf->data = malloc(initial_size * sizeof(char));
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    
    buf->data[0] = '\0';
    buf->len = 0;
    buf->max_len = initial_size;

    return buf;
}


void strbuf_free(StrBuf *buf)
{
    free(buf->data);
    free(buf);
}


static int strbuf_grow(StrBuf *buf, int minimum_size)
{
    int new_size = (int)(buf->max_len * 1.5);
    if (new_size < minimum_size)
        new_size = minimum_size;

    char *tmp = realloc(buf->data, new_size * sizeof(char));
    if (tmp == NULL)
        return -1;

    buf->data = tmp;
    buf->max_len = new_size;
  
    return 0;
}


int strbuf_append(StrBuf *buf, char const *data, int length)
{
    int desired_length = buf->len + length + 1;

    if (buf->max_len < desired_length)
        if (strbuf_grow(buf, desired_length) == -1)
            return -1;

    memcpy(buf->data + buf->len, data, length);
    buf->len += length;
    buf->data[buf->len] = '\0';
  
    return 0;
}


char *strbuf_to_cstr(StrBuf *buf)
{
    char *result = malloc(buf->len + 1);
    if (!result)
        return NULL;
        
    memcpy(result, buf->data, buf->len + 1);
    result[buf->len] = '\0';

    return result;
}
