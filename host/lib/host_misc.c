/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
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

#include "host_common.h"

#include "cryptolib.h"
#include "utility.h"
#include "vboot_common.h"


char* StrCopy(char* dest, const char* src, int dest_size) {
  strncpy(dest, src, dest_size);
  dest[dest_size - 1] = '\0';
  return dest;
}


uint8_t* ReadFile(const char* filename, uint64_t* size) {
  FILE* f;
  uint8_t* buf;

  f = fopen(filename, "rb");
  if (!f) {
    VBDEBUG(("Unable to open file %s\n", filename));
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  *size = ftell(f);
  rewind(f);

  buf = Malloc(*size);
  if (!buf) {
    fclose(f);
    return NULL;
  }

  if(1 != fread(buf, *size, 1, f)) {
    VBDEBUG(("Unable to read from file %s\n", filename));
    fclose(f);
    Free(buf);
    return NULL;
  }

  fclose(f);
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


int ReadFileInt(const char* filename) {
  char buf[64];
  int value;
  char* e = NULL;

  if (!ReadFileString(buf, sizeof(buf), filename))
    return -1;

  /* Convert to integer.  Allow characters after the int ("123 blah"). */
  value = strtol(buf, &e, 0);
  if (e == buf)
    return -1;  /* No characters consumed, so conversion failed */

  return value;
}


int ReadFileBit(const char* filename, int bitmask) {
  int value = ReadFileInt(filename);
  if (value == -1)
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
  }

  fclose(f);
  return 0;
}

void PrintPubKeySha1Sum(VbPublicKey* key) {
  uint8_t* buf = ((uint8_t *)key) + key->key_offset;
  uint64_t buflen = key->key_size;
  uint8_t* digest = DigestBuf(buf, buflen, SHA1_DIGEST_ALGORITHM);
  int i;
  for (i=0; i<SHA1_DIGEST_SIZE; i++)
    printf("%02x", digest[i]);
  Free(digest);
}
