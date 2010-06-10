/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Host functions for verified boot.
 */

/* TODO: change all 'return 0', 'return 1' into meaningful return codes */

#include <stdio.h>
#include <stdlib.h>

#include "host_common.h"

#include "cryptolib.h"
#include "utility.h"
#include "vboot_common.h"


uint8_t* ReadFile(const char* filename, uint64_t* size) {
  FILE* f;
  uint8_t* buf;

  f = fopen(filename, "rb");
  if (!f) {
    debug("Unable to open file %s\n", filename);
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
    debug("Unable to read from file %s\n", filename);
    fclose(f);
    Free(buf);
    return NULL;
  }

  fclose(f);
  return buf;
}
