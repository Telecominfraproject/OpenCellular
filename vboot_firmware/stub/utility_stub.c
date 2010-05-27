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
  fprintf(stderr, "WARNING: ");
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

void* Memcpy(void* dest, const void* src, size_t n) {
  return memcpy(dest, src, n);
}

void* Memset(void* dest, const uint8_t c, size_t n) {
  while (n--) {
    *((uint8_t*)dest++) = c;
  }
  return dest;
}

int SafeMemcmp(const void* s1, const void* s2, size_t n) {
  int match = 0;
  const unsigned char* us1 = s1;
  const unsigned char* us2 = s2;
  while (n--) {
    if (*us1++ != *us2++)
      match = 1;
  }

  return match;
}
