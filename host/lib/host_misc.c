/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for verified boot.
 */

/* TODO: change all 'return 0', 'return 1' into meaningful return codes */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "host_common.h"

#include "cryptolib.h"
#include "utility.h"
#include "vboot_common.h"


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
