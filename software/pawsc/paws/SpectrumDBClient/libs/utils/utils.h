/*
Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License.
*/

#ifndef UTILS_H_
#define UTILS_H_

#include "types.h"
#include "json-parser/json.h"

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
// winfows #define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

#define fatal_error(fmt, ...) { fprintf(stderr, "FATAL ERROR: " fmt "\n", ##__VA_ARGS__); exit(-1); }

#define UNUSED_PARAM(p) (void)p;

extern void free_and_null(void** data);

extern uint8_t* hexstrstr(uint8_t* src, uint32_t src_len, uint8_t* substr, uint32_t substr_len);



#endif // UTILS_H_
