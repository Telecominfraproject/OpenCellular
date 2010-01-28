/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of utility functions which call their linux-specific
 * equivalents.
 */

#include "utility.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

void* Memcpy(void* dest, const void* src, size_t n) {
  return memcpy(dest, src, n);
}

int SafeMemcmp(const void* s1, const void* s2, size_t n) {
  int match = 1;
  const unsigned char* us1 = s1;
  const unsigned char* us2 = s2;
  while (n--) {
    if (*us1++ != *us2++)
      match = 0;
    else
      match = 1;
  }

  return match;
}
