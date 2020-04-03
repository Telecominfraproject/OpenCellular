/*
 * Based on json-parser by James McLaughlin. See original license below.
 * For modifications see .c file.
 *
 * Copyright (C) 2012, 2013, 2014 James McLaughlin et al.  All rights reserved.
 * https://github.com/udp/json-parser
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include <inttypes.h>
#include <stdlib.h>


#define json_enable_comments  0x01


typedef enum
{
    json_object,
    json_array,
    json_integer,
    json_double,
    json_string,
    json_boolean,
    json_null
} json_type;


typedef struct _json_object_entry
{
     char * name;
     struct _json_value * value;
} json_object_entry;


typedef struct _json_value
{
    struct _json_value * parent;
    json_type type;

    union
    {
        int boolean;
        int64_t integer;
        double dbl;

        struct
        {
            unsigned int length;
            char * ptr; // null terminated
        } string;

        struct
        {
            unsigned int length;
            json_object_entry * values; // While in the 1st pass, the parser uses this value to store the number of bytes needed to store all the names that will be referenced in the json_object_entries. 
        } object;

        struct
        {
            unsigned int length;
            struct _json_value ** values;
        } array;
    } u;

    union
    {
        struct _json_value * next_alloc;
        void * object_mem;
    } _reserved;

    // Location of the value in the source JSON
    unsigned int line, col;
} json_value;


typedef char json_char;


json_value *json_parse(const char *json, size_t length);

#define json_error_max 128
json_value *json_parse_ex(const char *json,
                          size_t length,
                          char *error_buf);

void json_value_free(json_value *);
