#pragma once

/* This is a fake header to allow us to use the actual sysbios headers
 * for things such as mutexes and semaphores. It's a placeholder until
 * I can figure out a better way to do it (possibly target xdc for Linux)
 * - IG
 */

#include <stdint.h>

typedef int8_t xdc_Int8;
typedef uint8_t xdc_UInt8;
typedef int16_t xdc_Int16;
typedef uint16_t xdc_UInt16;
typedef int32_t xdc_Int32;
typedef uint32_t xdc_UInt32;
typedef int64_t xdc_Int64;
typedef uint64_t xdc_UInt64;

#define xdc__LONGLONG__
#define xdc__BITS8__
#define xdc__BITS16__
#define xdc__BITS32__
#define xdc__BITS64__
#define xdc__INT64__

/*
 *  ======== Bits<n> ========
 */
#ifdef xdc__BITS8__
typedef uint8_t xdc_Bits8;
#endif
#ifdef xdc__BITS16__
typedef uint16_t xdc_Bits16;
#endif
#ifdef xdc__BITS32__
typedef uint32_t xdc_Bits32;
#endif
#ifdef xdc__BITS64__
typedef uint64_t xdc_Bits64;
#endif

/*
 *  ======== [UI]Arg ========
 */
typedef intptr_t xdc_IArg;
typedef uintptr_t xdc_UArg;
