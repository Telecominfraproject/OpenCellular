/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of utility functions which call their linux-specific
 * equivalents.
 */

#define _STUB_IMPLEMENTATION_
#include "utility.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void error(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "ERROR: ");
  vfprintf(stderr, format, ap);
  va_end(ap);
  exit(1);
}

void debug(const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "DEBUG: ");
  vfprintf(stderr, format, ap);
  va_end(ap);
}

void* Malloc(size_t size) {
  void* p = malloc(size);
  if (!p) {
    /* Fatal Error. We must abort. */
    abort();
  }
  return p;
}

void Free(void* ptr) {
  free(ptr);
}

int Memcmp(const void* src1, const void* src2, size_t n) {
  return memcmp(src1, src2, n);
}

void* Memcpy(void* dest, const void* src, uint64_t n) {
  return memcpy(dest, src, (size_t)n);
}

void* Memset(void* d, const uint8_t c, uint64_t n) {
  uint8_t* dest = d; /* the only way to keep both cl and gcc happy */
  while (n--) {
    *dest++ = c;
  }
  return dest;
}


int SafeMemcmp(const void* s1, const void* s2, size_t n) {
  int result = 0;
  if (0 == n)
    return 1;

  const unsigned char* us1 = s1;
  const unsigned char* us2 = s2;
  /* Code snippet without data-dependent branch due to
   * Nate Lawson (nate@root.org) of Root Labs. */
  while (n--)
    result |= *us1++ ^ *us2++;

  return result != 0;
}
