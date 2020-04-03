/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#pragma once

typedef struct 
{
    char *data;     // Always NULL terminated.
    int len;        // Not including NULL terminator.
    int max_len;    // Does include NULL terminator.
} StrBuf;


StrBuf *strbuf_alloc();                                             // Returns NULL on failure.
void strbuf_free(StrBuf *buf);
int strbuf_append(StrBuf *buf, char const *data, int length);       // Returns -1 on failure, 0 on success.
char *strbuf_to_cstr(StrBuf *buf);                                  // Returns NULL on failure. On success, returned string is guaranteed to be NULL terminated.

// Helper macro for appending C strings. cstr must be NULL terminated.
// This is a macro rather than a function, so that the compiler has the
// opportunity to evaluate the strlen() at compile time if cstr is known
// at compile time.
#define strbuf_append_cstr(buf, cstr) strbuf_append(buf, cstr, strlen(cstr))
