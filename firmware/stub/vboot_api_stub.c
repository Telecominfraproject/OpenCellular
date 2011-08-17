/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of firmware-provided API functions.
 */

#define _STUB_IMPLEMENTATION_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "vboot_api.h"

/* disable MSVC warnings on unused arguments */
__pragma(warning (disable: 4100))


void VbExError(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "ERROR: ");
  vfprintf(stderr, format, ap);
  va_end(ap);
  exit(1);
}


void VbExDebug(const char* format, ...) {
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "DEBUG: ");
  vfprintf(stderr, format, ap);
  va_end(ap);
}


void* VbExMalloc(size_t size) {
  void* p = malloc(size);
  if (!p) {
    /* Fatal Error. We must abort. */
    abort();
  }
  return p;
}


void VbExFree(void* ptr) {
  free(ptr);
}


uint64_t VbExGetTimer(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (uint64_t)tv.tv_sec * 1000000 + (uint64_t)tv.tv_usec;
}


void VbExSleepMs(uint32_t msec) {
}


void VbExBeep(uint32_t msec, uint32_t frequency) {
}


VbError_t VbExNvStorageRead(uint8_t* buf) {
  return VBERROR_SUCCESS;
}


VbError_t VbExNvStorageWrite(const uint8_t* buf) {
  return VBERROR_SUCCESS;
}


VbError_t VbExHashFirmwareBody(VbCommonParams* cparams,
                               uint32_t firmware_index) {
  return VBERROR_SUCCESS;
}


VbError_t VbExDisplayInit(uint32_t* width, uint32_t* height) {
  return VBERROR_SUCCESS;
}


VbError_t VbExDisplayBacklight(uint8_t enable) {
  return VBERROR_SUCCESS;
}


VbError_t VbExDisplayScreen(uint32_t screen_type) {
  return VBERROR_SUCCESS;
}


VbError_t VbExDisplayImage(uint32_t x, uint32_t y,
                           void* buffer, uint32_t buffersize) {
  return VBERROR_SUCCESS;
}


VbError_t VbExDisplayDebugInfo(const char* info_str) {
  return VBERROR_SUCCESS;
}


uint32_t VbExKeyboardRead(void) {
  return 0;
}


uint32_t VbExIsShutdownRequested(void) {
  return 0;
}

VbError_t VbExDecompress(void *inbuf, uint32_t in_size,
                         uint32_t compression_type,
                         void *outbuf, uint32_t *out_size) {
  return VBERROR_SUCCESS;
}
