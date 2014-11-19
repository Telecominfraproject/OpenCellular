/* Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for verified boot.
 */

/* TODO: change all 'return 0', 'return 1' into meaningful return codes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cryptolib.h"
#include "host_common.h"
#include "vboot_common.h"


char* StrCopy(char* dest, const char* src, int dest_size) {
  strncpy(dest, src, dest_size);
  dest[dest_size - 1] = '\0';
  return dest;
}


uint8_t* ReadFile(const char* filename, uint64_t* sizeptr) {
  FILE* f;
  uint8_t* buf;
  uint64_t size;

  f = fopen(filename, "rb");
  if (!f) {
    VBDEBUG(("Unable to open file %s\n", filename));
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  rewind(f);

  buf = malloc(size);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  if(1 != fread(buf, size, 1, f)) {
    VBDEBUG(("Unable to read from file %s\n", filename));
    fclose(f);
    free(buf);
    return NULL;
  }

  fclose(f);
  if (sizeptr)
    *sizeptr = size;
  return buf;
}


char* ReadFileString(char* dest, int size, const char* filename) {
  char* got;
  FILE* f;

  f = fopen(filename, "rt");
  if (!f)
    return NULL;

  got = fgets(dest, size, f);
  fclose(f);
  return got;
}


int ReadFileInt(const char* filename, unsigned* value) {
  char buf[64];
  char* e = NULL;

  if (!ReadFileString(buf, sizeof(buf), filename))
    return -1;

  /* Convert to integer.  Allow characters after the int ("123 blah"). */
  *value = (unsigned)strtoul(buf, &e, 0);
  if (e == buf)
    return -1;  /* No characters consumed, so conversion failed */

  return 0;
}


int ReadFileBit(const char* filename, int bitmask) {
  unsigned value;
  if (ReadFileInt(filename, &value) < 0)
    return -1;
  else return (value & bitmask ? 1 : 0);
}


int WriteFile(const char* filename, const void *data, uint64_t size) {
  FILE *f = fopen(filename, "wb");
  if (!f) {
    VBDEBUG(("Unable to open file %s\n", filename));
    return 1;
  }

  if (1 != fwrite(data, size, 1, f)) {
    VBDEBUG(("Unable to write to file %s\n", filename));
    fclose(f);
    unlink(filename);  /* Delete any partial file */
    return 1;
  }

  fclose(f);
  return 0;
}
